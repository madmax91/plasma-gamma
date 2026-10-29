# Plasma Gamma

A system tray app for KDE Plasma (Wayland) that changes display gamma. It has global shortcuts you can rebind. Gamma raises or lowers shadows and midtones and leaves pure black and pure white alone, so dark scenes become readable without washing out the whole image.

Default shortcuts:

| Action | Shortcut |
|---|---|
| Increase gamma (brighter shadows) | `Ctrl+Meta+Alt+Up` |
| Decrease gamma | `Ctrl+Meta+Alt+Down` |
| Reset to 1.00 | `Ctrl+Meta+Alt+0` |

On the tray icon, scrolling adjusts gamma and a left click resets it. The right-click menu has step size, the on-screen display toggle, Start at Login, **Configure Shortcuts…** and Quit. You can also rebind the shortcuts in System Settings > Keyboard > Shortcuts > Plasma Gamma.

## How it works

KWin on Wayland has no gamma API. It does let you assign an ICC colour profile to each monitor, which is what System Settings > Display & Monitor > Color Profile does. Plasma Gamma uses that:

1. For every enabled SDR monitor, it writes a small ICC profile to `~/.local/share/plasma-gamma/` using lcms2. The profile keeps the colour primaries the monitor is already using (EDID, sRGB, or your own ICC profile). Its tone curve is changed so that KWin compensates by gamma `g`.
2. It applies the profile through libkscreen, the same library `kscreen-doctor` and System Settings use.
3. At gamma 1.00, or when you quit, log out or kill the app, each monitor goes back to exactly the profile it had before. If the app crashes, it restores the profiles the next time it starts.

Monitors with HDR turned on are skipped, because KWin uses a separate profile path for them. The tray tooltip lists any skipped monitors and why.

## Anti-cheat (Easy Anti-Cheat, BattlEye, etc.)

In short, this app works the same way as Night Light and the ICC profile setting in System Settings. It never comes near a game process. Here is what it does **not** do:

- It does not inject anything, hook functions, use `LD_PRELOAD`, or load a Vulkan layer into any process.
- It does not read or write another process's memory.
- It does not draw an overlay or capture the screen.
- It does not hook the keyboard. Shortcuts are registered with `kglobalaccel`, Plasma's own shortcut service, which KWin delivers.
- It does not access the GPU driver directly. The colour change happens inside KWin's compositing pipeline, after the game has finished rendering its frame.

From inside a Proton/Wine prefix, a game (and its anti-cheat) sees nothing different. It cannot tell this apart from you picking a different colour profile in System Settings.

**This is not a guarantee.** Only the anti-cheat vendor and the game's developer decide what gets you banned, and nobody else can promise otherwise. Some competitive games' terms of service also restrict "visual advantage" tools, whatever the technical method. Check the rules of the specific game if that matters to you. Technically, this sits in the same category as monitor OSD gamma or "black equalizer" settings, not with tools like ReShade that inject into the game.

## Requirements

- KDE Plasma 6 on **Wayland**. Developed against Plasma 6.8 / Frameworks 6.31 / Qt 6.12. Older Plasma 6 releases may lack the ICC API in libkscreen.
- A monitor that KWin reports as ICC-capable. Check with `kscreen-doctor -o`: the output should not say `ICC profile: incapable`.

## Building

### Arch / CachyOS / Manjaro

```bash
sudo pacman -S --needed git base-devel cmake extra-cmake-modules qt6-base \
    kcoreaddons kconfig kdbusaddons kglobalaccel kstatusnotifieritem kxmlgui \
    libkscreen lcms2
```

### Fedora (KDE spin)

```bash
sudo dnf install git gcc-c++ cmake extra-cmake-modules qt6-qtbase-devel \
    kf6-kcoreaddons-devel kf6-kconfig-devel kf6-kdbusaddons-devel \
    kf6-kglobalaccel-devel kf6-kstatusnotifieritem-devel kf6-kxmlgui-devel \
    libkscreen-devel lcms2-devel
```

### Kubuntu / KDE neon / Debian

```bash
sudo apt install git g++ cmake extra-cmake-modules qt6-base-dev \
    libkf6coreaddons-dev libkf6config-dev libkf6dbusaddons-dev \
    libkf6globalaccel-dev libkf6statusnotifieritem-dev libkf6xmlgui-dev \
    libkf6screen-dev liblcms2-dev
```

Only the Arch package names have been tested. The Fedora and Debian names are best-effort. If one doesn't resolve, search for the library name, e.g. `dnf search kscreen`.

### Build and install

```bash
git clone https://github.com/madmax91/plasma-gamma.git
cd plasma-gamma
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

Start it with **Plasma Gamma** from the app launcher, or run `plasma-gamma`. To start it automatically, tick **Start at Login** in the tray menu.

### Uninstall

Quit the app first so your monitors go back to their original profiles. Then:

```bash
sudo xargs rm < build/install_manifest.txt
rm -rf ~/.local/share/plasma-gamma ~/.config/plasma-gammarc ~/.local/state/plasma-gammastaterc \
    ~/.config/autostart/org.madmax91.plasma-gamma.desktop
```

## If something looks wrong

- **Colours look off while gamma is not 1.00:** the generated profile uses your monitor's EDID primaries when its profile source is EDID, and sRGB primaries when it's sRGB. Reset to 1.00 and compare. The tooltip shows what was applied to each monitor.
- **Stuck on a modified profile after a crash:** start the app again (it restores on startup), or open System Settings > Display & Monitor and set the Color Profile back to what it was.
- **Nothing happens when changing gamma:** check `kscreen-doctor -o` for `ICC profile: incapable`, and check the tray tooltip for skipped monitors.

## Settings

`~/.config/plasma-gammarc`:

```ini
[General]
Step=0.1      # also selectable from the tray menu
Min=0.5
Max=2.5
ShowOsd=true
```
