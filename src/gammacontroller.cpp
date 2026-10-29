// SPDX-License-Identifier: MIT
#include "gammacontroller.h"
#include "iccprofile.h"

#include <KConfigGroup>
#include <KScreen/Config>
#include <KScreen/ConfigMonitor>
#include <KScreen/GetConfigOperation>
#include <KScreen/Output>
#include <KScreen/SetConfigOperation>

#include <QDir>
#include <QStandardPaths>

#include <cmath>

using KScreen::Output;

static bool isNeutral(double gamma)
{
    return std::abs(gamma - 1.0) < 0.001;
}

GammaController::GammaController(QObject *parent)
    : QObject(parent)
    , m_state(KSharedConfig::openStateConfig(QStringLiteral("plasma-gammastaterc")))
{
    // Key repeat fires far faster than KWin can rebuild its colour pipeline
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(120);
    connect(&m_debounce, &QTimer::timeout, this, &GammaController::apply);
}

void GammaController::start()
{
    loadOriginals();
    auto *op = new KScreen::GetConfigOperation();
    connect(op, &KScreen::ConfigOperation::finished, this, [this](KScreen::ConfigOperation *op) {
        if (op->hasError()) {
            m_status = QStringLiteral("Cannot read display configuration: %1").arg(op->errorString());
            Q_EMIT statusChanged(m_status);
            return;
        }
        m_config = qobject_cast<KScreen::GetConfigOperation *>(op)->config();
        KScreen::ConfigMonitor::instance()->addConfig(m_config);

        // A previous run that died without restoring leaves its profiles in
        // KWin's saved output config; undo that before doing anything else.
        if (restoreOriginals()) {
            m_busy = true;
            auto *set = new KScreen::SetConfigOperation(m_config);
            connect(set, &KScreen::ConfigOperation::finished, this, &GammaController::onApplied);
        } else {
            removeStaleProfiles();
        }
        m_status = QStringLiteral("Gamma 1.00 (original colour profiles)");
        Q_EMIT statusChanged(m_status);
    });
}

double GammaController::gamma() const
{
    return m_gamma;
}

void GammaController::setGamma(double gamma)
{
    gamma = std::round(gamma * 100.0) / 100.0;
    if (gamma == m_gamma) {
        return;
    }
    m_gamma = gamma;
    Q_EMIT gammaChanged(m_gamma);
    m_debounce.start();
}

QString GammaController::status() const
{
    return m_status;
}

QString GammaController::profileDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/plasma-gamma");
}

void GammaController::apply()
{
    if (!m_config) {
        return;
    }
    if (m_busy) {
        m_pending = true;
        return;
    }

    if (isNeutral(m_gamma)) {
        restoreOriginals();
        m_activeProfiles.clear();
        m_status = QStringLiteral("Gamma 1.00 (original colour profiles)");
    } else {
        QStringList applied;
        QStringList skipped;
        QStringList profiles;
        const auto outputs = m_config->outputs();
        for (const auto &output : outputs) {
            if (!output->isConnected() || !output->isEnabled()) {
                continue;
            }
            const QString name = output->name();
            if (!(output->capabilities() & Output::Capability::IccProfile)) {
                skipped << name + QStringLiteral(" (no ICC support)");
                continue;
            }
            if (output->isHdrEnabled()) {
                skipped << name + QStringLiteral(" (HDR)");
                continue;
            }
            if (!m_originals.contains(name)) {
                m_originals.insert(name, {output->iccProfilePath(), static_cast<int>(output->colorProfileSource())});
                saveOriginals();
            }
            const Original &orig = m_originals.value(name);

            QString basePath;
            IccProfile::Chromaticity chroma = IccProfile::srgb();
            const auto source = static_cast<Output::ColorProfileSource>(orig.source);
            if (source == Output::ColorProfileSource::ICC && !orig.iccPath.isEmpty()) {
                basePath = orig.iccPath;
            } else if (source == Output::ColorProfileSource::EDID) {
                IccProfile::readEdidChromaticity(name, &chroma);
            }

            const QString path = QStringLiteral("%1/%2-%3.icc").arg(profileDir(), name, QString::number(m_gamma, 'f', 2));
            QString error;
            if (!IccProfile::write(path, m_gamma, basePath, chroma, &error)) {
                skipped << QStringLiteral("%1 (%2)").arg(name, error);
                continue;
            }
            output->setIccProfilePath(path);
            output->setColorProfileSource(Output::ColorProfileSource::ICC);
            applied << name;
            profiles << path;
        }
        m_activeProfiles = profiles;
        m_status = QStringLiteral("Gamma %1 on %2").arg(QString::number(m_gamma, 'f', 2), applied.isEmpty() ? QStringLiteral("no outputs") : applied.join(QStringLiteral(", ")));
        if (!skipped.isEmpty()) {
            m_status += QStringLiteral("\nSkipped: ") + skipped.join(QStringLiteral(", "));
        }
    }

    m_busy = true;
    auto *op = new KScreen::SetConfigOperation(m_config);
    connect(op, &KScreen::ConfigOperation::finished, this, &GammaController::onApplied);
    Q_EMIT statusChanged(m_status);
}

void GammaController::onApplied(KScreen::ConfigOperation *op)
{
    m_busy = false;
    if (op->hasError()) {
        m_status = QStringLiteral("KWin rejected the change: %1").arg(op->errorString());
        Q_EMIT statusChanged(m_status);
    }
    removeStaleProfiles();
    if (m_pending) {
        m_pending = false;
        apply();
    }
}

bool GammaController::restoreOriginals()
{
    if (m_originals.isEmpty() || !m_config) {
        return false;
    }
    // Outputs that are unplugged right now keep their entry, so the next
    // restore (or the next start) can still put them back.
    bool restored = false;
    const auto outputs = m_config->outputs();
    for (const auto &output : outputs) {
        const auto it = m_originals.constFind(output->name());
        if (it == m_originals.constEnd()) {
            continue;
        }
        output->setIccProfilePath(it->iccPath);
        output->setColorProfileSource(static_cast<Output::ColorProfileSource>(it->source));
        m_originals.erase(it);
        restored = true;
    }
    saveOriginals();
    return restored;
}

void GammaController::restoreBlocking()
{
    m_debounce.stop();
    if (!restoreOriginals()) {
        return;
    }
    KScreen::SetConfigOperation op(m_config);
    op.exec();
    m_activeProfiles.clear();
    removeStaleProfiles();
}

void GammaController::loadOriginals()
{
    m_originals.clear();
    KConfigGroup group(m_state, QStringLiteral("Originals"));
    const QStringList names = group.groupList();
    for (const QString &name : names) {
        const KConfigGroup g = group.group(name);
        m_originals.insert(name, {g.readEntry("IccProfilePath", QString()), g.readEntry("ColorProfileSource", 0)});
    }
}

void GammaController::saveOriginals()
{
    m_state->deleteGroup(QStringLiteral("Originals"));
    KConfigGroup group(m_state, QStringLiteral("Originals"));
    for (auto it = m_originals.constBegin(); it != m_originals.constEnd(); ++it) {
        KConfigGroup g = group.group(it.key());
        g.writeEntry("IccProfilePath", it->iccPath);
        g.writeEntry("ColorProfileSource", it->source);
    }
    m_state->sync();
}

void GammaController::removeStaleProfiles()
{
    QDir dir(profileDir());
    const QStringList files = dir.entryList({QStringLiteral("*.icc")}, QDir::Files);
    for (const QString &file : files) {
        const QString path = dir.filePath(file);
        if (!m_activeProfiles.contains(path)) {
            QFile::remove(path);
        }
    }
}
