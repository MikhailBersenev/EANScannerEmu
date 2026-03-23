# Building EANScannerEmu

This document describes how to compile the project from source on Linux, Windows, and macOS.

## Requirements

- **Qt**: 6.8 or newer (modules: `core`, `gui`, `widgets`, `multimedia`, `multimediawidgets`)
- **C++**: compiler with **C++17** support
- **qmake**: part of the Qt installation (often `qmake` or `qmake6` on Linux)

Platform-specific libraries are linked in `EANScannerEmuSrc.pro`:

| Platform | Extra dependencies |
|----------|---------------------|
| **Linux** | Development packages for **X11** and **Xtst** (`libX11`, `libXtst`, typically `libx11` / `libxtst` on distros) |
| **Windows** | Qt for MSVC or MinGW; `res.rc` supplies the application icon |
| **macOS** | **ApplicationServices** and **Carbon** (handled via `-framework` in the project file) |

---

## Qt Creator

1. Install **Qt 6** and **Qt Creator** (e.g. [Qt Online Installer](https://www.qt.io/download-qt-installer)).
2. **File** → **Open File or Project…** → choose **`EANScannerEmuSrc.pro`** in the repository root.
3. When prompted, select a **kit** (Qt 6.x, matching compiler: GCC, Clang, MSVC, or MinGW on Windows).
4. **Build** → **Build Project** (or toolbar hammer / **Ctrl+B** on Linux & Windows, **⌘B** on macOS).
5. **Run** (green play) to start **`EANScannerEmuSrc`**.

**Tips**

- **Projects** (left sidebar) → **Build** → set **Build directory** if you want a separate out-of-tree folder (shadow build).
- **Projects** → **Run** → **Run configuration**: working directory is usually fine as default; the app loads optional data from the user’s home as documented in the main README.
- On **Linux**, install X11/Xtst development packages *before* building (see below), or the link step will fail even if Qt Creator is set up correctly.

---

## Linux

### Install dependencies (examples)

- **Debian / Ubuntu**: `sudo apt install build-essential qt6-base-dev qt6-multimedia-dev libx11-dev libxtst-dev`
- **Fedora**: `sudo dnf install gcc-c++ qt6-qtbase-devel qt6-qtmultimedia-devel libX11-devel libXtst-devel`
- **Arch**: `sudo pacman -S base-devel qt6-base qt6-multimedia libx11 libxtst`

Ensure `qmake` points to Qt 6 (e.g. `/usr/bin/qmake6`).

### Configure and build

From the repository root:

```bash
mkdir -p build && cd build
qmake6 ../EANScannerEmuSrc.pro CONFIG+=release   # or: qmake ../EANScannerEmuSrc.pro CONFIG+=release
make -j"$(nproc)"
```

The executable is **`EANScannerEmuSrc`** in the build directory.

Debug build:

```bash
qmake6 ../EANScannerEmuSrc.pro CONFIG+=debug
make -j"$(nproc)"
```

### Runtime notes

- **X11**: no extra setup for synthetic key events beyond X11 permissions where applicable.
- **Wayland**: install and configure **`ydotool`** (and any daemon it requires) so the app can inject keys; see the main [README](README.md).

---

## Windows

1. Install [Qt 6](https://www.qt.io/download) (MSVC or MinGW kit).
2. Open **Qt Creator** → open `EANScannerEmuSrc.pro`, select a kit, build; **or** use a **Developer Command Prompt** / Qt shell:

```bat
mkdir build
cd build
qmake ..\EANScannerEmuSrc.pro CONFIG+=release
nmake
```

(MinGW: `mingw32-make` instead of `nmake`.)

Output: **`EANScannerEmuSrc.exe`** in the build folder. Deploy Qt DLLs with `windeployqt` if you run the binary outside Qt Creator.

---

## macOS

1. Install Qt 6 (Qt Online Installer or Homebrew).
2. From a terminal:

```bash
mkdir -p build && cd build
qmake ../EANScannerEmuSrc.pro CONFIG+=release
make -j"$(sysctl -n hw.ncpu)"
```

Grant **Input Monitoring** or **Accessibility** if the system asks, so keystrokes can be sent to other applications.

---

## AppImage (Linux, x86_64)

Portable single-file packages are built with [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) and **linuxdeploy-plugin-qt**.

### Extra tools

- `curl`, `patchelf`, `squashfs-tools` (`mksquashfs`)
- ImageMagick: `magick` or `convert` (to resize `icon.png` to a size accepted by linuxdeploy)

### Command

From the repository root:

```bash
export QMAKE=/usr/bin/qmake6
./packaging/linux/build-appimage.sh
```

If your Qt 6 `qmake` is not `/usr/bin/qmake6`, set `QMAKE` to its full path.

The script:

- Builds a **release** binary in `dist-appimage/build/`
- Assembles **`dist-appimage/AppDir`**
- Writes the **`.AppImage`** under **`dist-appimage/`**

The output filename includes **`VERSION`**. By default `VERSION` is taken from `git describe`; override if needed:

```bash
VERSION=1.2.3 ./packaging/linux/build-appimage.sh
```

### Environment variables

| Variable | Purpose |
|----------|---------|
| `QMAKE` | Path to Qt 6 `qmake` |
| `VERSION` | Version string embedded in the AppImage name |
| `NO_STRIP` | Defaults to `1`. The strip tool shipped inside linuxdeploy can fail on modern ELF (e.g. `.relr.dyn`); keeping `NO_STRIP=1` avoids that. Set `NO_STRIP=0` only if you know your toolchain is compatible. |

On some distributions (e.g. with KDE Qt packages), optional **`kimg_*`** image plugins would pull extra libraries. The script uses a staged plugin tree **without** those plugins so the bundle does not require libraries such as `libjxrglue` at build time.

---

## Troubleshooting

- **`qmake`: command not found** — Add Qt’s `bin` directory to `PATH`, or call the full path to `qmake`/`qmake6`.
- **Linux: missing X11/Xtst** — Install the `-dev` / `-devel` packages for `libX11` and `libXtst`.
- **AppImage build fails inside linuxdeploy** — Ensure Qt 6 is used consistently; try `NO_STRIP=1` (default in the script) and see the notes above.
