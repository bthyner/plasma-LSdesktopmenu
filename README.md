# plasma-containmentactions-customdesktopmenu

Custom desktop menu for Plasma 6 design like the old Litestep menu

<img width="250px" title="menu" alt="menu" src=".assets/menu.png"> <img width="350px" title="config" alt="config" src=".assets/config.png">

## Build and install

### Prerequisites

- extra-cmake-modules >= 6.0.0
- kdeplasma-addons >= 6.0.0



### Build

```
cd src
cmake --fresh -B ../_build .
cd ../_build
make clean
make
```

### Install / Update

```
chmod 755 bin/plasma/containmentactions/matmoul-customdesktopmenu.so
sudo cp bin/plasma/containmentactions/matmoul-customdesktopmenu.so /usr/lib/qt6/plugins/plasma/containmentactions
```

## Other

- When you update the library, KWIN crash and restart with the new library.
- Favorites is not yet implemented.
- expand setting page and include guide
- icon only side bar
- tool tip previews 
- expand syntax and things that can be added
- include serch
- add desktop menues


# Changelog – Custom Desktop Menu (LiteStep-style)


## [Unreleased] – 2026-08-08

### Added
- **Live folder trees** via new `{folder}` directive
  - Syntax examples:
    - `{folder}	/home/user`
    - `{folder}	/home/user	depth=3`
    - `{folder}	/home/user	depth=3	showhidden=false`
  - `depth=N` – maximum recursion depth (default: 3)
  - `showhidden=true|false` – show/hide dotfiles (default: false)
  - Clicking a folder opens it in the default file manager
  - Hovering still shows the submenu for navigation

- **Configurable maximum items per folder**
  - New setting in the configuration dialog: “Maximum items per folder”
  - Default: 300
  - Prevents the menu from freezing on very large directories
  - Shows a “… (X more items)” entry when the limit is reached

- Better file icons using `QMimeDatabase` instead of a generic “unknown” icon

### Fixed
- **Critical crash** when using `{programs}`
  - `KServiceGroup::group()` could return null on Plasma 6
  - Calling `->entries()` on a null pointer crashed plasmashell
  - Added proper null/validity check + warning

- Memory leaks
  - All `QMenu` objects created for submenus are now tracked and properly deleted
  - Fixed leaking `KProcess` instances (now uses `KProcess::startDetached()`)

- Plugin not appearing in Mouse Actions after installation
  - Correct install path and `kbuildsycoca6` refresh issues resolved during setup

### Changed
- Removed obsolete `init(const KConfigGroup&)` method
  - No longer exists in current Plasma 6 `ContainmentActions` base class
  - Eliminates “marked override but does not override” build error

- Modernized code for Qt 6 / Plasma 6 best practices
  - Replaced deprecated `foreach` with range-based for loops
  - Consistent use of `QStringLiteral` and `QLatin1Char`/`QLatin1String`
  - Cleaner lambda captures
  - Removed unused `m_group` member

- Improved default menu configuration for Plasma 6 service group paths

### Technical Notes
- Configuration is still stored as plain text (easy to edit and share)
- All new settings are properly saved/restored via `KConfigGroup`
- The plugin remains fully compatible with the original MatMoul design while adding the LiteStep-style folder browsing

## Sources

- https://github.com/MatMoul/plasma-containmentactions-customdesktopmenu/tree/plasma5
- https://invent.kde.org/plasma/plasma-workspace/-/tree/ea415539fc6256494d5c12296a6216e522e12b0a/containmentactions
- https://invent.kde.org/plasma/plasma-workspace/-/blob/master/applets/kicker/plugin/kastatsfavoritesmodel.cpp?ref_type=heads
