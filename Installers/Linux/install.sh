#!/bin/bash
# ==========================================================================
#  Polyphase Engine - Linux Install Script (tarball fallback)
#
#  Installs Polyphase Engine to /opt/polyphase/ and creates system integration
#  files (wrapper script, desktop entry, MIME type, icon).
#
#  Usage: sudo ./install.sh
# ==========================================================================

set -e

INSTALL_DIR="/opt/polyphase"
WRAPPER="/usr/local/bin/polyphase-editor"
DESKTOP_DIR="/usr/share/applications"
MIME_DIR="/usr/share/mime/packages"
ICON_DIR="/usr/share/icons/hicolor/128x128/apps"
PROFILE_SCRIPT="/etc/profile.d/polyphase.sh"

# Check for root
if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: This script must be run as root (use sudo)."
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Check that the polyphase/ distribution tree exists next to this script
if [ ! -d "$SCRIPT_DIR/polyphase" ]; then
    echo "ERROR: polyphase/ directory not found next to this script."
    echo "       Expected: $SCRIPT_DIR/polyphase/"
    exit 1
fi

echo "============================================"
echo " Polyphase Engine - Linux Installer"
echo "============================================"
echo ""

# --- Install engine files ---
echo "Installing to $INSTALL_DIR..."
if [ -d "$INSTALL_DIR" ]; then
    echo "  Removing previous installation..."
    rm -rf "$INSTALL_DIR"
fi
cp -r "$SCRIPT_DIR/polyphase" "$INSTALL_DIR"

# --- Set permissions ---
echo "Setting permissions..."
chmod +x "$INSTALL_DIR/PolyphaseEditor"
mkdir -p "$INSTALL_DIR/Engine/Saves"
chmod 777 "$INSTALL_DIR/Engine/Saves"

# --- Create wrapper script ---
echo "Creating wrapper script at $WRAPPER..."
cat > "$WRAPPER" << 'WRAPPER_EOF'
#!/bin/bash
cd /opt/polyphase
exec /opt/polyphase/PolyphaseEditor "$@"
WRAPPER_EOF
chmod +x "$WRAPPER"

# --- Install desktop entry ---
echo "Installing desktop entry..."
mkdir -p "$DESKTOP_DIR"
cp "$SCRIPT_DIR/polyphase-editor.desktop" "$DESKTOP_DIR/polyphase-editor.desktop"

# --- Install MIME type ---
echo "Installing MIME type..."
mkdir -p "$MIME_DIR"
cp "$SCRIPT_DIR/polyphase-editor.xml" "$MIME_DIR/polyphase-editor.xml"

# --- Install icon ---
echo "Installing icon..."
mkdir -p "$ICON_DIR"
if [ -f "$INSTALL_DIR/PolyphaseLogo_128.png" ]; then
    cp "$INSTALL_DIR/PolyphaseLogo_128.png" "$ICON_DIR/polyphase-editor.png"
fi

# --- Set POLYPHASE_PATH environment variable ---
echo "Setting POLYPHASE_PATH environment variable..."
cat > "$PROFILE_SCRIPT" << 'PROFILE_EOF'
# Polyphase Engine environment
export POLYPHASE_PATH="/opt/polyphase"
PROFILE_EOF
chmod 644 "$PROFILE_SCRIPT"

# --- Update system caches ---
echo "Updating system caches..."
if command -v update-desktop-database > /dev/null 2>&1; then
    update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
fi
if command -v update-mime-database > /dev/null 2>&1; then
    update-mime-database /usr/share/mime 2>/dev/null || true
fi
if command -v gtk-update-icon-cache > /dev/null 2>&1; then
    gtk-update-icon-cache /usr/share/icons/hicolor 2>/dev/null || true
fi

echo ""
echo "============================================"
echo " Installation complete!"
echo " Run: polyphase-editor"
echo ""
echo " POLYPHASE_PATH has been set to $INSTALL_DIR"
echo " (Log out and back in to apply)"
echo "============================================"
