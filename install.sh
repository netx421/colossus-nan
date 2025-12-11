#!/usr/bin/env bash
set -e

# Build artifact name from Makefile
BIN_BUILD="COLOSSUS-NAN"
# Name of the installed executable
BIN_INSTALL="COLOSSUS-NAN"

echo "[CN-007] INITIALIZING INSTALLATION SEQUENCE..."
sleep 1

# --------------------------------------------------------------------
#  PACKAGE ACQUISITION
# --------------------------------------------------------------------
echo "[CN-007] ACQUIRING REQUIRED PACKAGES..."

sudo pacman -S --needed --noconfirm \
    base-devel \
    gtk3 \
    webkit2gtk-4.1 \
    mpv \
    yt-dlp \
    pkgconf

echo "[CN-007] SYSTEM DEPENDENCIES VERIFIED."

# --------------------------------------------------------------------
#  BUILD PROCESS
# --------------------------------------------------------------------
echo "[CN-007] COMMENCING BUILD OF NETWORK ACCESS NODE..."

make clean || true
make

if [ ! -f "$BIN_BUILD" ]; then
    echo "[CN-007] ERROR: Build failed. Binary '$BIN_BUILD' not found."
    exit 1
fi

echo "[CN-007] BUILD SUCCESSFUL. BINARY READY: $BIN_BUILD"

# --------------------------------------------------------------------
#  INSTALLATION PROCEDURE
# --------------------------------------------------------------------
echo "[CN-007] INSTALLING SYSTEM FILES..."

# Install resource directory
sudo mkdir -p /usr/local/share/colossus-nan/resources
sudo cp -r resources/* /usr/local/share/colossus-nan/resources/

# Install executable (rename COLOSSUS-NAN -> nan)
sudo cp "$BIN_BUILD" /usr/local/bin/"$BIN_INSTALL"
sudo chmod +x /usr/local/bin/"$BIN_INSTALL"

# Install application icon (from resources/icon.png)
if [ -f resources/icon.png ]; then
    echo "[CN-007] INSTALLING APPLICATION ICON..."
    sudo mkdir -p /usr/local/share/icons/hicolor/256x256/apps
    sudo cp resources/icon.png \
        /usr/local/share/icons/hicolor/256x256/apps/colossus-nan.png
else
    echo "[CN-007] WARNING: resources/icon.png not found; skipping icon install."
fi

# Install .desktop entry for drun / app menus
if [ -f colossus-nan.desktop ]; then
    echo "[CN-007] INSTALLING DESKTOP ENTRY..."
    sudo cp colossus-nan.desktop /usr/share/applications/colossus-nan.desktop
else
    echo "[CN-007] WARNING: colossus-nan.desktop not found; skipping desktop entry install."
fi

echo "[CN-007] INSTALLATION COMPLETE."
echo "[CN-007] EXECUTABLE: /usr/local/bin/$BIN_INSTALL"
echo "[CN-007] RESOURCES:  /usr/local/share/colossus-nan/resources"
echo "[CN-007] DESKTOP ENTRY: /usr/share/applications/colossus-nan.desktop"
echo "[CN-007] YOU MAY NOW LAUNCH VIA: 'COLOSSUS-NAN' or from drun/app menu."
echo "[CN-007] END OF LINE."

