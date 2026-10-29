// SPDX-License-Identifier: MIT
#include "trayapp.h"
#include "gammacontroller.h"

#include <KActionCollection>
#include <KConfigGroup>
#include <KGlobalAccel>
#include <KShortcutsDialog>
#include <KStatusNotifierItem>

#include <QActionGroup>
#include <QApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QStandardPaths>

#include <algorithm>
#include <cmath>

static const QString s_icon = QStringLiteral("preferences-desktop-display-color");

TrayApp::TrayApp(QObject *parent)
    : QObject(parent)
    , m_settings(KSharedConfig::openConfig(QStringLiteral("plasma-gammarc")))
    , m_controller(new GammaController(this))
    , m_tray(new KStatusNotifierItem(QStringLiteral("plasma-gamma"), this))
    , m_actions(new KActionCollection(this, QStringLiteral("plasma-gamma")))
    , m_menu(new QMenu())
{
    m_actions->setComponentDisplayName(QStringLiteral("Plasma Gamma"));

    // Global shortcuts go through kglobalaccel, which KWin owns: the app
    // never reads the keyboard itself. Autoloading keeps whatever the user
    // set in the shortcuts dialog or System Settings > Shortcuts.
    const auto addGlobal = [this](const QString &id, const QString &text, const QString &icon, QKeySequence seq, auto slot) {
        QAction *action = m_actions->addAction(id);
        action->setText(text);
        action->setIcon(QIcon::fromTheme(icon));
        KGlobalAccel::self()->setDefaultShortcut(action, {seq});
        KGlobalAccel::self()->setShortcut(action, {seq});
        connect(action, &QAction::triggered, this, slot);
        return action;
    };
    QAction *up = addGlobal(QStringLiteral("increase"), QStringLiteral("Increase Gamma"), QStringLiteral("go-up"),
                            QKeySequence(Qt::CTRL | Qt::META | Qt::ALT | Qt::Key_Up), [this] { step(+1); });
    QAction *down = addGlobal(QStringLiteral("decrease"), QStringLiteral("Decrease Gamma"), QStringLiteral("go-down"),
                              QKeySequence(Qt::CTRL | Qt::META | Qt::ALT | Qt::Key_Down), [this] { step(-1); });
    QAction *reset = addGlobal(QStringLiteral("reset"), QStringLiteral("Reset Gamma"), QStringLiteral("edit-undo"),
                               QKeySequence(Qt::CTRL | Qt::META | Qt::ALT | Qt::Key_0), [this] {
                                   m_controller->setGamma(1.0);
                                   showOsd();
                               });

    m_header = m_menu->addSection(QString());
    m_menu->addAction(up);
    m_menu->addAction(down);
    m_menu->addAction(reset);
    m_menu->addSeparator();

    QMenu *stepMenu = m_menu->addMenu(QStringLiteral("Step Size"));
    auto *stepGroup = new QActionGroup(stepMenu);
    const double currentStep = KConfigGroup(m_settings, QStringLiteral("General")).readEntry("Step", 0.1);
    for (double value : {0.02, 0.05, 0.1, 0.2}) {
        QAction *a = stepMenu->addAction(QString::number(value, 'f', 2));
        a->setCheckable(true);
        a->setChecked(std::abs(value - currentStep) < 0.001);
        stepGroup->addAction(a);
        connect(a, &QAction::triggered, this, [this, value] {
            KConfigGroup(m_settings, QStringLiteral("General")).writeEntry("Step", value);
            m_settings->sync();
        });
    }

    QAction *osd = m_menu->addAction(QStringLiteral("Show On-Screen Display"));
    osd->setCheckable(true);
    osd->setChecked(KConfigGroup(m_settings, QStringLiteral("General")).readEntry("ShowOsd", true));
    connect(osd, &QAction::toggled, this, [this](bool on) {
        KConfigGroup(m_settings, QStringLiteral("General")).writeEntry("ShowOsd", on);
        m_settings->sync();
    });

    QAction *autostart = m_menu->addAction(QStringLiteral("Start at Login"));
    autostart->setCheckable(true);
    autostart->setChecked(autostartEnabled());
    connect(autostart, &QAction::toggled, this, &TrayApp::setAutostart);

    QAction *shortcuts = m_menu->addAction(QIcon::fromTheme(QStringLiteral("configure-shortcuts")), QStringLiteral("Configure Shortcuts…"));
    connect(shortcuts, &QAction::triggered, this, [this] {
        KShortcutsDialog::showDialog(m_actions, KShortcutsEditor::LetterShortcutsAllowed, nullptr);
    });

    m_menu->addSeparator();
    QAction *quit = m_menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), QStringLiteral("Quit"));
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);

    m_tray->setTitle(QStringLiteral("Plasma Gamma"));
    m_tray->setIconByName(s_icon);
    m_tray->setCategory(KStatusNotifierItem::Hardware);
    m_tray->setStatus(KStatusNotifierItem::Active);
    m_tray->setStandardActionsEnabled(false);
    m_tray->setContextMenu(m_menu);
    // Left click resets; scrolling over the icon adjusts.
    connect(m_tray, &KStatusNotifierItem::activateRequested, this, [this] {
        m_controller->setGamma(1.0);
        showOsd();
    });
    connect(m_tray, &KStatusNotifierItem::scrollRequested, this, [this](int delta, Qt::Orientation orientation) {
        if (orientation == Qt::Vertical && delta != 0) {
            step(delta > 0 ? +1 : -1);
        }
    });

    connect(m_controller, &GammaController::gammaChanged, this, &TrayApp::updateTooltip);
    connect(m_controller, &GammaController::statusChanged, this, &TrayApp::updateTooltip);
    connect(qApp, &QApplication::aboutToQuit, m_controller, &GammaController::restoreBlocking);

    updateTooltip();
    m_controller->start();
}

GammaController *TrayApp::controller() const
{
    return m_controller;
}

void TrayApp::step(int direction)
{
    const KConfigGroup general(m_settings, QStringLiteral("General"));
    const double stepSize = general.readEntry("Step", 0.1);
    const double min = general.readEntry("Min", 0.5);
    const double max = general.readEntry("Max", 2.5);
    m_controller->setGamma(std::clamp(m_controller->gamma() + direction * stepSize, min, max));
    showOsd();
}

void TrayApp::updateTooltip()
{
    const QString value = QString::number(m_controller->gamma(), 'f', 2);
    m_header->setText(QStringLiteral("Gamma %1").arg(value));
    m_tray->setToolTip(s_icon, QStringLiteral("Gamma %1").arg(value), m_controller->status());
}

void TrayApp::showOsd()
{
    if (!KConfigGroup(m_settings, QStringLiteral("General")).readEntry("ShowOsd", true)) {
        return;
    }
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                      QStringLiteral("/org/kde/osdService"),
                                                      QStringLiteral("org.kde.osdService"),
                                                      QStringLiteral("showText"));
    msg << s_icon << QStringLiteral("Gamma %1").arg(QString::number(m_controller->gamma(), 'f', 2));
    QDBusConnection::sessionBus().asyncCall(msg);
}

QString TrayApp::autostartPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/autostart/org.madmax91.plasma-gamma.desktop");
}

bool TrayApp::autostartEnabled()
{
    return QFile::exists(autostartPath());
}

void TrayApp::setAutostart(bool enabled)
{
    const QString path = autostartPath();
    if (!enabled) {
        QFile::remove(path);
        return;
    }
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QStringLiteral("[Desktop Entry]\n"
                               "Type=Application\n"
                               "Name=Plasma Gamma\n"
                               "Exec=%1\n"
                               "Icon=%2\n"
                               "OnlyShowIn=KDE;\n"
                               "X-KDE-autostart-phase=2\n")
                    .arg(QApplication::applicationFilePath(), s_icon)
                    .toUtf8());
    }
}
