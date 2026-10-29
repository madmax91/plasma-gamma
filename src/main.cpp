// SPDX-License-Identifier: MIT
#include "trayapp.h"

#include <KAboutData>
#include <KDBusService>

#include <QApplication>
#include <QIcon>
#include <QSocketNotifier>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

// Self-pipe so SIGTERM/SIGINT (logout, `kill`, Ctrl+C) go through a normal
// QApplication::quit() and the original colour profiles get restored.
static int s_signalFd[2];

static void onSignal(int)
{
    const char c = 1;
    [[maybe_unused]] auto n = ::write(s_signalFd[0], &c, 1);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    KAboutData about(QStringLiteral("plasma-gamma"),
                     QStringLiteral("Plasma Gamma"),
                     QStringLiteral(PROJECT_VERSION),
                     QStringLiteral("Display gamma control for KDE Plasma (Wayland) via KWin colour profiles"),
                     KAboutLicense::MIT);
    about.setDesktopFileName(QStringLiteral("org.madmax91.plasma-gamma"));
    KAboutData::setApplicationData(about);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-display-color")));

    KDBusService service(KDBusService::Unique);

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s_signalFd) == 0) {
        auto *notifier = new QSocketNotifier(s_signalFd[1], QSocketNotifier::Read, &app);
        QObject::connect(notifier, &QSocketNotifier::activated, &app, [] {
            char c;
            [[maybe_unused]] auto n = ::read(s_signalFd[1], &c, 1);
            QApplication::quit();
        });
        std::signal(SIGTERM, onSignal);
        std::signal(SIGINT, onSignal);
        std::signal(SIGHUP, onSignal);
    }

    TrayApp tray;
    return app.exec();
}
