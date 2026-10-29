// SPDX-License-Identifier: MIT
#pragma once

#include <KSharedConfig>

#include <QObject>

class GammaController;
class KActionCollection;
class KStatusNotifierItem;
class QAction;
class QMenu;

class TrayApp : public QObject
{
    Q_OBJECT

public:
    explicit TrayApp(QObject *parent = nullptr);

    GammaController *controller() const;

private:
    void step(int direction);
    void updateTooltip();
    void showOsd();
    void setAutostart(bool enabled);
    static bool autostartEnabled();
    static QString autostartPath();

    KSharedConfigPtr m_settings;
    GammaController *m_controller;
    KStatusNotifierItem *m_tray;
    KActionCollection *m_actions;
    QMenu *m_menu;
    QAction *m_header;
};
