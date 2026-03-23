#!/usr/bin/env bash
# Сборка AppImage (Qt 6, x86_64). Требования: qmake из Qt6, patchelf, squashfs-tools,
# ImageMagick (magick или convert) для ресайза иконки.
# Опционально: переменная QMAKE, например QMAKE=~/Qt/6.8.2/gcc_64/bin/qmake
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

OUT_DIR="${ROOT}/dist-appimage"
BUILD_DIR="${OUT_DIR}/build"
APPDIR="${OUT_DIR}/AppDir"
LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
PLUGIN_QT_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"

QMAKE_BIN="${QMAKE:-qmake}"
if ! command -v "$QMAKE_BIN" &>/dev/null; then
  echo "Не найден qmake. Укажите Qt 6: export QMAKE=/path/to/Qt/6.x/gcc_64/bin/qmake" >&2
  exit 1
fi

QT_VERSION="$("$QMAKE_BIN" -query QT_VERSION)"
if [[ "${QT_VERSION%%.*}" -lt 6 ]]; then
  echo "Ожидается Qt 6, найдено: $QT_VERSION ($QMAKE_BIN)" >&2
  exit 1
fi

rm -rf "$OUT_DIR"
mkdir -p "$BUILD_DIR" "$APPDIR/usr/bin"

echo "==> qmake + make (release)"
(
  cd "$BUILD_DIR"
  "$QMAKE_BIN" "$ROOT/EANScannerEmuSrc.pro" CONFIG+=release
  make -j"$(nproc)"
)

if [[ ! -f "$BUILD_DIR/EANScannerEmuSrc" ]]; then
  echo "Не найден бинарник: $BUILD_DIR/EANScannerEmuSrc" >&2
  exit 1
fi

cp "$BUILD_DIR/EANScannerEmuSrc" "$APPDIR/usr/bin/"
chmod +x "$APPDIR/usr/bin/EANScannerEmuSrc"

LINUXDEPLOY="${LINUXDEPLOY:-$OUT_DIR/linuxdeploy-x86_64.AppImage}"
PLUGIN_QT="${LINUXDEPLOY_PLUGIN_QT:-$OUT_DIR/linuxdeploy-plugin-qt-x86_64.AppImage}"

fetch_tool() {
  local url="$1" dest="$2"
  if [[ -x "$dest" ]]; then
    return 0
  fi
  echo "==> загрузка $(basename "$dest")"
  curl -fsSL -o "$dest" "$url"
  chmod +x "$dest"
}

fetch_tool "$LINUXDEPLOY_URL" "$LINUXDEPLOY"
fetch_tool "$PLUGIN_QT_URL" "$PLUGIN_QT"

# На дистрибутивах с KDE (Arch и др.) в imageformats лежат плагины kimg_*; linuxdeploy-plugin-qt
# пытается стянуть все зависимости, в т.ч. libjxrglue и др., которых может не быть в системе.
# Копируем дерево плагинов и убираем kimg_* — приложению для GUI достаточно стандартных форматов Qt.
PLUGIN_STAGE="$OUT_DIR/qt-plugins-staging"
SRC_PLUGINS="$("$QMAKE_BIN" -query QT_INSTALL_PLUGINS)"
rm -rf "$PLUGIN_STAGE"
cp -a "$SRC_PLUGINS" "$PLUGIN_STAGE"
shopt -s nullglob
rm -f "$PLUGIN_STAGE/imageformats/kimg_"*.so
shopt -u nullglob

QMAKE_WRAPPER="$OUT_DIR/qmake-linuxdeploy.sh"
{
  echo '#!/usr/bin/env bash'
  echo 'set -euo pipefail'
  echo "REAL_QMAKE=$(printf '%q' "$QMAKE_BIN")"
  echo "STAGE_PLUGINS=$(printf '%q' "$PLUGIN_STAGE")"
  echo 'if [[ "${1:-}" == "-query" ]] && [[ $# -eq 1 ]]; then'
  echo '  "$REAL_QMAKE" -query | while IFS= read -r line; do'
  echo '    if [[ "$line" == QT_INSTALL_PLUGINS:* ]]; then'
  echo '      echo "QT_INSTALL_PLUGINS:$STAGE_PLUGINS"'
  echo '    else'
  echo '      printf "%s\n" "$line"'
  echo '    fi'
  echo '  done'
  echo 'else'
  echo '  exec "$REAL_QMAKE" "$@"'
  echo 'fi'
} >"$QMAKE_WRAPPER"
chmod +x "$QMAKE_WRAPPER"

# linuxdeploy принимает только фиксированные размеры PNG (не 1024×1024 и т.п.)
APPIMAGE_ICON="$OUT_DIR/icon-appimage.png"
if command -v magick &>/dev/null; then
  magick "$ROOT/icon.png" -resize 256x256 "$APPIMAGE_ICON"
elif command -v convert &>/dev/null; then
  convert "$ROOT/icon.png" -resize 256x256 "$APPIMAGE_ICON"
else
  echo "Для ресайза icon.png нужен ImageMagick (magick или convert)." >&2
  exit 1
fi

export ARCH="${ARCH:-x86_64}"
export VERSION="${VERSION:-$(git -C "$ROOT" describe --tags --always 2>/dev/null || echo "0.0.0")}"
# Плагин Qt вызывает qmake -query; подменяем только QT_INSTALL_PLUGINS на очищенное дерево плагинов
export QMAKE="$QMAKE_WRAPPER"
export PATH="$(dirname "$QMAKE_BIN"):$PATH"
# strip из состава linuxdeploy часто старее системного и не понимает ELF с .relr.dyn (GCC 15+),
# из‑за чего сборка падает. По умолчанию отключаем strip; для уменьшения размера: NO_STRIP=0 на системе с новым binutils (риск — снова ошибка strip).
export NO_STRIP="${NO_STRIP:-1}"

echo "==> linuxdeploy (Qt), VERSION=$VERSION NO_STRIP=$NO_STRIP"
(
  cd "$OUT_DIR"
  APPIMAGE_EXTRACT_AND_RUN=1 \
    "$LINUXDEPLOY" --appdir "$APPDIR" \
    -e "$APPDIR/usr/bin/EANScannerEmuSrc" \
    -d "$ROOT/packaging/linux/EANScannerEmu.desktop" \
    -i "$APPIMAGE_ICON" \
    --icon-filename EANScannerEmu \
    --plugin qt \
    --output appimage
)

echo "==> готово:"
ls -la "$OUT_DIR"/*.AppImage 2>/dev/null || ls -la "$OUT_DIR"
