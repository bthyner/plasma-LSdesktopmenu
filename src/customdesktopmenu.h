#pragma once

#include <QMenu>
#include <KServiceGroup>
#include <plasma/containmentactions.h>

#include "ui_config.h"

class QAction;

/**
 * CustomDesktopMenu
 *
 * A Plasma ContainmentAction that provides a fully configurable
 * right-click menu on the desktop (inspired by LiteStep).
 *
 * Supports:
 *  - Static .desktop entries
 *  - Custom commands
 *  - Nested submenus ([menu] / [end])
 *  - Dynamic application categories ({programs})
 *  - Live folder trees ({folder})
 */
class CustomDesktopMenu : public Plasma::ContainmentActions
{
    Q_OBJECT

public:
    explicit CustomDesktopMenu(QObject *parent, const QVariantList &args);
    ~CustomDesktopMenu() override;

    // Called by Plasma when the user right-clicks the desktop
    QList<QAction *> contextualActions() override;

    // Configuration UI
    QWidget *createConfigurationInterface(QWidget *parent) override;
    void configurationAccepted() override;

    // Load / save settings
    void restore(const KConfigGroup &config) override;
    void save(KConfigGroup &config) override;

private:
    // --- Menu construction state ---------------------------------

    // Top-level actions that will be returned to Plasma
    QList<QAction *> m_actions;

    // Stack of currently open submenus while we are building the menu.
    // The last entry is the menu we are currently adding items into.
    QList<QMenu *> m_menuList;

    // All QMenu objects we create. We keep them so we can delete them
    // the next time contextualActions() is called (prevents memory leaks).
    QList<QMenu *> m_createdMenus;

    // --- Configuration -------------------------------------------

    // The raw text of the menu definition (edited by the user)
    QString m_menuConfig;

    // Whether to show application name or generic name from .desktop files
    bool m_showAppsByName = true;

    // UI form for the configuration dialog
    Ui::Config m_ui;
    
    // Maximum items shown in a folder menu
    int m_maxFolderEntries = 300;  

    // --- Helper methods ------------------------------------------

    // Returns the built-in default menu text
    QString getDefaultConfig();

    // Parses m_menuConfig and builds the action/menu tree
    void parseConfig();

    // Adds an action to the current menu (or to the top level)
    void addAction(QAction *action);

    // Fills the current menu with applications from a KServiceGroup path
    void fillPrograms(const QString &path);

    // Recursively builds a live folder tree
    // depth = maximum recursion levels, showHidden = show dotfiles
    void fillFolder(const QString &path,
                    int maxDepth = 3,
                    bool showHidden = false,
                    int currentDepth = 0);
};