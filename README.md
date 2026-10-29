# Plasma Gamma

A small background app for **KDE Plasma (Wayland)** that changes display gamma with keyboard shortcuts or from its system tray icon.

Gamma brightens or darkens shadows and midtones while pure black and pure white stay where they are. Dark scenes in games and videos become readable without the washed-out look you get from raising brightness.

---

## Contents

1. [Requirements](#1-requirements)
2. [Prerequisites: what to install](#2-prerequisites-what-to-install)
3. [Compiling from source](#3-compiling-from-source)
4. [Using it](#4-using-it)
5. [Settings](#5-settings)
6. [Updating and uninstalling](#6-updating-and-uninstalling)
7. [Troubleshooting](#7-troubleshooting)
8. [How it works](#8-how-it-works)
9. [Anti-cheat (Easy Anti-Cheat, BattlEye, ...)](#9-anti-cheat-easy-anti-cheat-battleye-)

---

## 1. Requirements

- **KDE Plasma 6 in a Wayland session.** It was developed against Plasma 6.8 / KDE Frameworks 6.31 / Qt 6.12. Older Plasma 6 releases may not have the colour-profile API it needs; see [Troubleshooting](#7-troubleshooting).
- **A monitor KWin can apply a colour profile to.** Check with:

  ```bash
  kscreen-doctor -o
  ```

  Each monitor you want to adjust must **not** show `ICC profile: incapable`. Almost every real desktop monitor qualifies. Virtual and remote-desktop displays usually don't.

## 2. Prerequisites: what to install

| What | Why it's needed |
|---|---|
| **git** | Downloads the source code |
| **C++ compiler** (g++ or clang, C++20) | Compiles the program |
| **CMake** ≥ 3.22 | Build system |
| **extra-cmake-modules** (ECM) | KDE's CMake helpers (install paths, compiler settings) |
| **pkg-config** / pkgconf | Locates lcms2 |
| **Qt 6** base (Widgets, DBus) | GUI toolkit, tray menu, D-Bus |
| **KF6 CoreAddons** | App metadata |
| **KF6 Config** | Saves settings |
| **KF6 DBusAddons** | Allows only one copy to run |
| **KF6 GlobalAccel** | Global keyboard shortcuts (through Plasma, not by reading the keyboard) |
| **KF6 StatusNotifierItem** | The system tray icon |
| **KF6 XmlGui** | The "Configure Shortcuts" dialog |
| **libkscreen** (KF6Screen) | Talks to KWin to set each monitor's colour profile |
| **lcms2** (Little CMS) | Generates the ICC colour profiles |

Pick your distribution and copy the whole command.

### Arch Linux / CachyOS / EndeavourOS / Manjaro

```bash
sudo pacman -S --needed git base-devel cmake extra-cmake-modules pkgconf qt6-base \
    kcoreaddons kconfig kdbusaddons kglobalaccel kstatusnotifieritem kxmlgui \
    libkscreen lcms2
```

### Fedora (KDE Plasma Desktop edition)

```bash
sudo dnf install git gcc-c++ cmake extra-cmake-modules pkgconf-pkg-config qt6-qtbase-devel \
    kf6-kcoreaddons-devel kf6-kconfig-devel kf6-kdbusaddons-devel \
    kf6-kglobalaccel-devel kf6-kstatusnotifieritem-devel kf6-kxmlgui-devel \
    libkscreen-devel lcms2-devel
```

### Kubuntu / KDE neon / Debian testing

```bash
sudo apt install git g++ cmake extra-cmake-modules pkg-config qt6-base-dev \
    libkf6coreaddons-dev libkf6config-dev libkf6dbusaddons-dev \
    libkf6globalaccel-dev libkf6statusnotifieritem-dev libkf6xmlgui-dev \
    libkscreen-dev liblcms2-dev
```

### openSUSE Tumbleweed

```bash
sudo zypper install git gcc-c++ cmake kf6-extra-cmake-modules pkgconf-pkg-config qt6-base-devel \
    kf6-kcoreaddons-devel kf6-kconfig-devel kf6-kdbusaddons-devel \
    kf6-kglobalaccel-devel kf6-kstatusnotifieritem-devel kf6-kxmlgui-devel \
    libkscreen6-devel liblcms2-devel
```

> Only the Arch package list has been installed and built against end to end. The Fedora `libkscreen-devel` package was confirmed to provide `KF6Screen`. Other names follow each distribution's usual KF6 naming. If one isn't found, search for the library: `dnf search kscreen`, `apt search kscreen`, `zypper search kscreen`.
>
> Debian **stable** and older Ubuntu LTS releases ship Plasma versions that may be too old. Use Debian testing, a current Kubuntu or KDE neon.

## 3. Compiling from source

```bash
# 1. Download the source
git clone https://github.com/madmax91/plasma-gamma.git
cd plasma-gamma

# 2. Configure (the program is installed to /usr/local)
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release

# 3. Compile
cmake --build build -j"$(nproc)"

# 4. Install (puts the program and its app launcher entry in place)
sudo cmake --install build
```

Step 2 prints a summary. If anything under "required packages not found" shows up, a prerequisite from section 2 is missing.

**To run it without installing:**

```bash
./build/bin/plasma-gamma
```

(If that path doesn't exist on your system, try `./build/plasma-gamma`.)

## 4. Using it

### Starting

- Open the app launcher and start **Plasma Gamma**, or run `plasma-gamma` in a terminal.
- A display icon appears in the system tray. If you don't see it, open the tray's expand arrow (^). Plasma may have tucked it under "hidden icons".
- To start it automatically when you log in, right-click the tray icon and tick **Start at Login**.

### Keyboard shortcuts (defaults)

| Shortcut | Action |
|---|---|
| `Ctrl + Meta + Alt + Up` | Increase gamma (brighter shadows) |
| `Ctrl + Meta + Alt + Down` | Decrease gamma (darker shadows) |
| `Ctrl + Meta + Alt + 0` | Reset to 1.00 (normal) |

`Meta` is the Windows / Super key. Each press shows the new value in Plasma's on-screen display.

### Tray icon

| Action | Result |
|---|---|
| Scroll over the icon | Raise / lower gamma |
| Left-click | Reset to 1.00 |
| Hover | Shows the current gamma and which monitors it is applied to (or skipped, and why) |
| Right-click | Menu (below) |

**Right-click menu:**

- **Increase / Decrease / Reset Gamma**
- **Step Size**: how much one press changes gamma (0.02, 0.05, 0.10, 0.20)
- **Show On-Screen Display**: turn the pop-up value display on or off
- **Start at Login**
- **Configure Shortcuts…**: choose your own keys
- **Quit**: puts every monitor back to how it was, then exits

### Changing the shortcuts

Either:

- Right-click the tray icon → **Configure Shortcuts…**, click a shortcut, press the new key combination, then **OK**. Or:
- **System Settings → Keyboard → Shortcuts → Plasma Gamma**.

Your choice is remembered across restarts.

### Gamma values

- **1.00**: normal. Your monitors use exactly the colour profile they had before.
- **Above 1.00** (e.g. 1.30): lifts shadows. This is what you usually want in dark games.
- **Below 1.00** (e.g. 0.80): deepens shadows.
- The allowed range is 0.50–2.50 by default (see [Settings](#5-settings)).

Gamma always **starts at 1.00** when the app launches. Values are not remembered between runs, so nothing stays changed by surprise.

## 5. Settings

Most settings live in the tray menu. Everything is stored in `~/.config/plasma-gammarc`:

```ini
[General]
Step=0.1       # change per key press (also in the tray menu)
Min=0.5        # lowest allowed gamma
Max=2.5        # highest allowed gamma
ShowOsd=true   # on-screen display on each change
```

To edit `Min`/`Max`, quit the app, edit the file and start it again.

## 6. Updating and uninstalling

**Update to the latest version:**

```bash
cd plasma-gamma
git pull
cmake --build build -j"$(nproc)"
sudo cmake --install build
```

Then quit and restart the app from the tray.

**Uninstall:**

```bash
# Quit the app from the tray first, so monitors return to their original profiles
cd plasma-gamma
sudo xargs rm < build/install_manifest.txt
rm -rf ~/.local/share/plasma-gamma \
       ~/.config/plasma-gammarc \
       ~/.local/state/plasma-gammastaterc \
       ~/.config/autostart/org.madmax91.plasma-gamma.desktop
```

## 7. Troubleshooting

**CMake says a package is missing.**
Install the matching package from section 2. The name it prints (e.g. `KF6GlobalAccel`) maps to the table there.

**Compile errors mentioning `colorProfileSource`, `setIccProfilePath` or `ColorProfileSource`.**
Your distribution's Plasma / libkscreen is too old for the colour-profile API. Update Plasma or use a newer distribution release.

**Pressing the shortcut shows the on-screen value but the screen doesn't change.**
Hover the tray icon. The tooltip lists skipped monitors and the reason:
- `no ICC support`: KWin can't apply a profile to that output (see section 1).
- `HDR`: monitors with HDR on are skipped on purpose. Turn HDR off in System Settings → Display & Monitor to use it there.

**The shortcut does nothing at all.**
Another app may own that key combination. Pick different keys under **Configure Shortcuts…**.

**Colours look slightly different (not just brighter/darker) while gamma isn't 1.00.**
The profile reuses the colour space your monitor was set to (EDID, sRGB or your own ICC profile). Report it with your monitor model and the "Color Profile" setting from System Settings → Display & Monitor.

**The app crashed and the screen stayed changed.**
Start it again. On startup it restores the original profiles automatically. If that doesn't work, open System Settings → Display & Monitor and set **Color Profile** back to what it was (usually "Built-in" / EDID or "None").

**See what it's doing:** run it from a terminal (`plasma-gamma`) and watch for messages.

## 8. How it works

KWin on Wayland has no gamma control, and it doesn't support the Wayland protocol that tools like gammastep use. It does let you assign an **ICC colour profile** to each monitor; that's the System Settings → Display & Monitor → Color Profile option. Plasma Gamma uses that:

1. For each enabled SDR monitor, it builds a small ICC profile (with lcms2) in `~/.local/share/plasma-gamma/`. The profile has the same colour primaries the monitor was already using. Only its tone curve changes, raised to the power `g`. KWin compensates for that curve, and the visible result is gamma `g`.
2. It hands the profile to KWin through libkscreen, the same library System Settings and `kscreen-doctor` use.
3. Before changing a monitor, it saves that monitor's original profile setting to `~/.local/state/plasma-gammastaterc`. The original comes back at gamma 1.00, when you quit, at logout, or on the next start after a crash.

## 9. Anti-cheat (Easy Anti-Cheat, BattlEye, ...)

**What it does not do:**

- It does not inject anything into games: no DLLs, `LD_PRELOAD`, Vulkan layers or hooks.
- It does not read or write any other program's memory.
- It does not draw an overlay or capture the screen.
- It does not read the keyboard. Shortcuts are registered with Plasma's own shortcut service (`kglobalaccel`), and KWin delivers them.
- It does not access the GPU driver directly.

The colour change happens **inside KWin**, after the game has finished drawing its frame. It's the same mechanism as Plasma's Night Light or choosing a colour profile in System Settings. A game running under Proton/Wine, and its anti-cheat, has no way to tell it apart from you picking a different colour profile.

**This is not a guarantee.** Only the anti-cheat vendor and the game's developer decide what gets an account banned, and nobody else can promise otherwise. Some competitive games' rules also restrict "visual advantage" settings in general. If that matters to you, check the rules of the specific game. Technically, this sits in the same category as your monitor's own gamma / "black equalizer" setting, not with tools like ReShade that inject into the game.

---

License: MIT (see [LICENSE](LICENSE)).
