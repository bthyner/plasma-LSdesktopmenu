/*
 * Custom Desktop Menu – Plasma Containment Action
 *
 * Robust version with:
 *  - Null-safe KServiceGroup handling
 *  - Live {folder} support (depth + showhidden)
 *  - Proper QMenu lifetime management (no leaks)
 *  - Modern Qt 6 style
 */

#include "customdesktopmenu.h"

#include <KConfigGroup>
#include <KDesktopFile>
#include <KIO/ApplicationLauncherJob>
#include <KPluginFactory>
#include <KProcess>
#include <KService>
#include <KServiceGroup>

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QUrl>

CustomDesktopMenu::CustomDesktopMenu(QObject *parent, const QVariantList &args)
    : Plasma::ContainmentActions(parent, args)
{
}

CustomDesktopMenu::~CustomDesktopMenu()
{
    // Safety net – normally cleaned in contextualActions()
    qDeleteAll(m_createdMenus);
    m_createdMenus.clear();
}

QList<QAction *> CustomDesktopMenu::contextualActions()
{
    // Clean previous run completely
    qDeleteAll(m_actions);
    m_actions.clear();

    qDeleteAll(m_createdMenus);
    m_createdMenus.clear();
    m_menuList.clear();

    parseConfig();
    return m_actions;
}

QWidget *CustomDesktopMenu::createConfigurationInterface(QWidget *parent)
{
    QWidget *widget = new QWidget(parent);
    m_ui.setupUi(widget);
    widget->setWindowTitle(i18nc("plasma_containmentactions_customdesktopmenu",
                                 "Configure Custom Desktop Menu Plugin"));
    widget->setFixedWidth(660);
    widget->setFixedHeight(400);
    m_ui.configData->setPlainText(m_menuConfig);
    m_ui.showAppsByName->setChecked(m_showAppsByName);
    m_ui.maxFolderEntries->setValue(m_maxFolderEntries);
    return widget;
}

void CustomDesktopMenu::configurationAccepted()
{
    m_menuConfig = m_ui.configData->document()->toPlainText();
    m_showAppsByName = m_ui.showAppsByName->isChecked();
    m_maxFolderEntries = m_ui.maxFolderEntries->value();
}

void CustomDesktopMenu::restore(const KConfigGroup &config)
{
    m_menuConfig = config.readEntry(QStringLiteral("menuConfig"), getDefaultConfig());
    m_showAppsByName = config.readEntry(QStringLiteral("showAppsByName"), true);
    m_maxFolderEntries = config.readEntry(QStringLiteral("maxFolderEntries"), 300);
}

void CustomDesktopMenu::save(KConfigGroup &config)
{
    config.writeEntry(QStringLiteral("menuConfig"), m_menuConfig);
    config.writeEntry(QStringLiteral("showAppsByName"), m_showAppsByName);
    config.writeEntry(QStringLiteral("maxFolderEntries"), m_maxFolderEntries);
}

QString CustomDesktopMenu::getDefaultConfig()
{
    QString cfg;
    cfg += QStringLiteral("{favorites}\n");
    cfg += QStringLiteral("-\n");
    cfg += QStringLiteral("[menu]\tApplications\tkde\n");
    cfg += QStringLiteral("{programs}\n");
    cfg += QStringLiteral("[end]\n");
    cfg += QStringLiteral("-\n");
    cfg += QStringLiteral("/usr/share/applications/org.kde.dolphin.desktop\n");
    cfg += QStringLiteral("/usr/share/applications/org.kde.kate.desktop\n");
    cfg += QStringLiteral("#/usr/share/applications/org.kde.kcalc.desktop\n");
    cfg += QStringLiteral("-\n");
    cfg += QStringLiteral("[menu]\tSystem\tconfigure-shortcuts\n");
    cfg += QStringLiteral("{programs}\tSettings/\n");
    cfg += QStringLiteral("-\n");
    cfg += QStringLiteral("{programs}\tSystem/\n");
    cfg += QStringLiteral("[end]\n");
    cfg += QStringLiteral("[menu]\tExit\tsystem-shutdown\n");
    cfg += QStringLiteral("Lock\tsystem-lock-screen\tqdbus6 org.kde.KWin /ScreenSaver Lock\n");
    cfg += QStringLiteral("Disconnect\tsystem-log-out\tqdbus6 org.kde.LogoutPrompt /LogoutPrompt promptLogout\n");
    cfg += QStringLiteral("Switch User\tsystem-switch-user\tqdbus6 org.kde.KWin /ScreenSaver org.kde.screensaver.SwitchUser\n");
    cfg += QStringLiteral("-\n");
    cfg += QStringLiteral("Sleep\tsystem-suspend\tqdbus6 org.freedesktop.PowerManagement /org/freedesktop/PowerManagement Suspend\n");
    cfg += QStringLiteral("Hibernate\tsystem-suspend-hibernate\tqdbus6 org.freedesktop.PowerManagement /org/freedesktop/PowerManagement Hibernate\n");
    cfg += QStringLiteral("-\n");
    cfg += QStringLiteral("Restart\tsystem-reboot\tqdbus6 org.kde.LogoutPrompt /LogoutPrompt promptReboot\n");
    cfg += QStringLiteral("Shut down\tsystem-shutdown\tqdbus6 org.kde.LogoutPrompt /LogoutPrompt promptShutDown\n");
    cfg += QStringLiteral("[end]\n");
    cfg += QStringLiteral("-\n");
    cfg += QStringLiteral("/usr/share/applications/org.kde.plasma-systemmonitor.desktop\n");
    cfg += QStringLiteral("/usr/share/applications/org.kde.konsole.desktop\n");
    return cfg;
}

void CustomDesktopMenu::parseConfig()
{
    if (m_menuConfig.isEmpty()) {
        return;
    }

    const QStringList configLines = m_menuConfig.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    for (const QString &cfgLine : configLines) {
        if (cfgLine.startsWith(QLatin1Char('#'))) {
            continue;
        }

        QAction *action = nullptr;

        if (cfgLine.startsWith(QLatin1Char('-'))) {
            // Separator
            action = new QAction(this);
            action->setSeparator(true);

        } else if (cfgLine.startsWith(QLatin1String("[menu]"))) {
            // Begin submenu
            const QStringList cfgParts = cfgLine.split(QLatin1Char('\t'), Qt::SkipEmptyParts);
            QString text;
            QIcon icon;

            if (cfgParts.size() > 1) {
                text = cfgParts.at(1);
                text.replace(QLatin1Char('&'), QLatin1String("&&"));
            }
            if (cfgParts.size() > 2) {
                icon = QIcon::fromTheme(cfgParts.at(2));
            }

            QMenu *subMenu = new QMenu();
            m_createdMenus.append(subMenu);          // track for later deletion

            action = new QAction(icon, text, this);
            action->setMenu(subMenu);
            addAction(action);
            m_menuList.append(subMenu);
            action = nullptr;                        // already added

        } else if (cfgLine.startsWith(QLatin1String("[end]"))) {
            // End submenu
            if (!m_menuList.isEmpty()) {
                m_menuList.removeLast();
            }

        } else if (cfgLine.endsWith(QLatin1String(".desktop"))) {
            // Launch a .desktop file
            if (KDesktopFile::isDesktopFile(cfgLine)) {
                KDesktopFile desktopFile(cfgLine);
                QString text = desktopFile.readName();
                if (!m_showAppsByName && !desktopFile.readGenericName().isEmpty()) {
                    text = desktopFile.readGenericName();
                }

                action = new QAction(QIcon::fromTheme(desktopFile.readIcon()), text, this);
                action->setData(cfgLine);

                connect(action, &QAction::triggered, this, [desktopPath = cfgLine]() {
                    KService::Ptr service = KService::serviceByDesktopPath(desktopPath);
                    if (service) {
                        auto *job = new KIO::ApplicationLauncherJob(service);
                        job->start();
                    }
                });
            } else {
                action = new QAction(cfgLine, this);
            }

        } else if (cfgLine.startsWith(QLatin1String("{programs}"))) {
            // Dynamic applications menu
            const QStringList cfgParts = cfgLine.split(QLatin1Char('\t'), Qt::SkipEmptyParts);
            QString path = QStringLiteral("/");
            if (cfgParts.size() > 1) {
                path = cfgParts.at(1);
            }
            fillPrograms(path);

        } else if (cfgLine == QLatin1String("{favorites}")) {
            // Not implemented yet
            qDebug() << "CustomDesktopMenu: {favorites} is not implemented";

        } else if (cfgLine.startsWith(QLatin1String("{folder}"))) {
            // Live folder tree
            // Syntax: {folder}	/path	depth=3	showhidden=false
            const QStringList parts = cfgLine.split(QLatin1Char('\t'), Qt::SkipEmptyParts);
            if (parts.size() < 2) {
                qWarning() << "CustomDesktopMenu: {folder} requires a path";
                continue;
            }

            const QString folderPath = parts.at(1);
            int maxDepth = 3;
            bool showHidden = false;

            for (int i = 2; i < parts.size(); ++i) {
                const QString &opt = parts.at(i);
                if (opt.startsWith(QLatin1String("depth="))) {
                    bool ok = false;
                    const int d = opt.mid(6).toInt(&ok);
                    if (ok && d > 0) {
                        maxDepth = d;
                    }
                } else if (opt.startsWith(QLatin1String("showhidden="))) {
                    const QString val = opt.mid(11).toLower();
                    showHidden = (val == QLatin1String("true") || val == QLatin1String("1"));
                }
            }

            fillFolder(folderPath, maxDepth, showHidden);

        } else {
            // Custom command: Label	icon	command
            const QStringList cfgParts = cfgLine.split(QLatin1Char('\t'), Qt::SkipEmptyParts);
            if (cfgParts.isEmpty()) {
                continue;
            }

            QString text = cfgParts.at(0);
            text.replace(QLatin1Char('&'), QLatin1String("&&"));

            QIcon icon;
            if (cfgParts.size() > 1) {
                icon = QIcon::fromTheme(cfgParts.at(1));
            }

            action = new QAction(icon, text, this);

            if (cfgParts.size() > 2) {
                const QString cmd = cfgParts.at(2);
                connect(action, &QAction::triggered, this, [cmd]() {
                    KProcess::startDetached(cmd);
                });
            }
        }

        addAction(action);
    }
}

void CustomDesktopMenu::addAction(QAction *action)
{
    if (!action) {
        return;
    }

    if (m_menuList.isEmpty()) {
        m_actions.append(action);
    } else {
        m_menuList.last()->addAction(action);
    }
}

void CustomDesktopMenu::fillPrograms(const QString &path)
{
    QString normalized = path;
    if (!normalized.isEmpty() && !normalized.endsWith(QLatin1Char('/'))) {
        normalized += QLatin1Char('/');
    }

    KServiceGroup::Ptr root = KServiceGroup::group(normalized);

    // Critical null check – prevents the original crash
    if (!root || !root->isValid()) {
        qWarning() << "CustomDesktopMenu: invalid or missing service group:" << path;
        return;
    }

    const KServiceGroup::List list = root->entries(true, true, true);

    for (const KSycocaEntry::Ptr &entry : list) {
        if (entry->isType(KST_KService)) {
            if (!KDesktopFile::isDesktopFile(entry->entryPath())) {
                continue;
            }

            KDesktopFile desktopFile(entry->entryPath());
            QString text = entry->name();
            if (!m_showAppsByName && !desktopFile.readGenericName().isEmpty()) {
                text = desktopFile.readGenericName();
            }

            auto *action = new QAction(QIcon::fromTheme(desktopFile.readIcon()), text, this);
            action->setData(entry->entryPath());

            connect(action, &QAction::triggered, this, [desktopPath = entry->entryPath()]() {
                KService::Ptr service = KService::serviceByDesktopPath(desktopPath);
                if (service) {
                    auto *job = new KIO::ApplicationLauncherJob(service);
                    job->start();
                }
            });

            addAction(action);

        } else if (entry->isType(KST_KServiceGroup)) {
            const auto group = static_cast<KServiceGroup *>(entry.data());
            if (group->childCount() == 0) {
                continue;
            }

            QMenu *subMenu = new QMenu();
            m_createdMenus.append(subMenu);

            auto *action = new QAction(QIcon::fromTheme(group->icon()), group->caption(), this);
            action->setMenu(subMenu);
            addAction(action);

            m_menuList.append(subMenu);
            fillPrograms(entry->name());
            m_menuList.removeLast();

        } else if (entry->isType(KST_KServiceSeparator)) {
            auto *action = new QAction(this);
            action->setSeparator(true);
            addAction(action);
        }
    }
}

void CustomDesktopMenu::fillFolder(const QString &path, int maxDepth,
                                   bool showHidden, int currentDepth)
{
    if (currentDepth >= maxDepth) {
        return;
    }

    QDir dir(path);
    if (!dir.exists()) {
        qWarning() << "CustomDesktopMenu: folder does not exist:" << path;
        return;
    }

    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (showHidden) {
        filters |= QDir::Hidden;
    }

    const QFileInfoList entries = dir.entryInfoList(
        filters, QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);

    // Simple protection against extremely large directories
    const int maxEntries = m_maxFolderEntries;
    int count = 0;

    QMimeDatabase mimeDb;

    for (const QFileInfo &info : entries) {
        if (++count > maxEntries) {
            auto *more = new QAction(tr("… (%1 more items)").arg(entries.size() - maxEntries), this);
            more->setEnabled(false);
            addAction(more);
            break;
        }

        if (info.isDir()) {
            QMenu *subMenu = new QMenu();
            m_createdMenus.append(subMenu);

            auto *action = new QAction(QIcon::fromTheme(QStringLiteral("folder")),
                                       info.fileName(), this);
            action->setMenu(subMenu);

            // Clicking the folder opens it
            const QString folderPath = info.absoluteFilePath();
            connect(action, &QAction::triggered, this, [folderPath]() {
                QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
            });

            addAction(action);

            m_menuList.append(subMenu);
            fillFolder(folderPath, maxDepth, showHidden, currentDepth + 1);
            m_menuList.removeLast();

        } else if (info.isFile()) {
            // Better icon via MIME type
            const QIcon icon = QIcon::fromTheme(
                mimeDb.mimeTypeForFile(info).iconName(),
                QIcon::fromTheme(QStringLiteral("unknown")));

            auto *action = new QAction(icon, info.fileName(), this);

            const QString filePath = info.absoluteFilePath();
            connect(action, &QAction::triggered, this, [filePath]() {
                QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
            });

            addAction(action);
        }
    }
}

K_PLUGIN_CLASS_WITH_JSON(CustomDesktopMenu, "plasma-containmentactions-customdesktopmenu.json")

#include "customdesktopmenu.moc"