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
echo "[CN-007] DETECTING HOST DISTRIBUTION..."

if command -v pacman &>/dev/null; then
    echo "[CN-007] ARCH LINUX / MANJARO DETECTED"
    sudo pacman -S --needed --noconfirm \
        base-devel \
        gtk3 \
        webkit2gtk-4.1 \
        mpv \
        yt-dlp \
        pkgconf

elif command -v apt &>/dev/null; then
    echo "[CN-007] DEBIAN / UBUNTU / LINUX MINT DETECTED"
    sudo apt update
    sudo apt install -y \
        build-essential \
        libgtk-3-dev \
        libwebkit2gtk-4.0-dev \
        mpv \
        yt-dlp \
        pkg-config

elif command -v dnf &>/dev/null; then
    echo "[CN-007] FEDORA DETECTED"
    sudo dnf install -y \
        gtk3-devel \
        webkit2gtk4.0-devel \
        mpv \
        yt-dlp \
        pkgconf \
        gcc-c++

elif command -v zypper &>/dev/null; then
    echo "[CN-007] OPENSUSE DETECTED"
    sudo zypper install -y \
        gtk3-devel \
        webkit2gtk3-devel \
        mpv \
        yt-dlp \
        pkgconf \
        gcc-c++

else
    echo "[CN-007] ERROR: Unsupported Linux distribution."
    echo "Install the following dependencies manually:"
    echo "    GTK3, WebKitGTK, mpv, yt-dlp, pkg-config, C++ build tools"
    exit 1
fi


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

# Install application colossus-nan (from resources/colossus-nan.png)
if [ -f resources/colossus-nan.png ]; then
    echo "[CN-007] INSTALLING APPLICATION colossus-nan..."
    sudo mkdir -p /usr/local/share/colossus-nans/hicolor/256x256/apps
    sudo cp resources/colossus-nan.png \
        /usr/local/share/colossus-nans/hicolor/256x256/apps/colossus-nan.png
else
    echo "[CN-007] WARNING: resources/colossus-nan.png not found; skipping colossus-nan install."
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

