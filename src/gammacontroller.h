// SPDX-License-Identifier: MIT
#pragma once

#include <KScreen/Types>
#include <KSharedConfig>

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTimer>

namespace KScreen
{
class ConfigOperation;
}

// Applies a gamma curve to every SDR output by swapping in a generated ICC
// profile through KWin's own output-management API (the same path System
// Settings > Display uses). Nothing touches other processes or the GPU.
class GammaController : public QObject
{
    Q_OBJECT

public:
    explicit GammaController(QObject *parent = nullptr);

    void start();

    double gamma() const;
    void setGamma(double gamma);

    // Puts every output back on the colour profile it had before we touched
    // it. Blocks until KWin has accepted the change; used on quit.
    void restoreBlocking();

    QString status() const;

Q_SIGNALS:
    void gammaChanged(double gamma);
    void statusChanged(const QString &status);

private:
    struct Original {
        QString iccPath;
        int source;
    };

    void apply();
    void onApplied(KScreen::ConfigOperation *op);
    bool restoreOriginals();
    void loadOriginals();
    void saveOriginals();
    void removeStaleProfiles();
    QString profileDir() const;

    KScreen::ConfigPtr m_config;
    KSharedConfigPtr m_state;
    QHash<QString, Original> m_originals;
    QStringList m_activeProfiles;
    double m_gamma = 1.0;
    bool m_busy = false;
    bool m_pending = false;
    QTimer m_debounce;
    QString m_status;
};
