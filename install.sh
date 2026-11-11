#!/usr/bin/env bash
#
# Zephys Terminal - Installation & Uninstallation Script
#
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

show_help() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS]

Install or uninstall Zephys Terminal.

Options:
  --system        Install system-wide to /usr/local (requires root/sudo)
  --user          Install for current user only to ~/.local
  --uninstall     Uninstall Zephys Terminal from system or user
  -h, --help      Show this help message

Auto-detection:
  If run as root (e.g. 'sudo ./install.sh'), system-wide mode is selected by default.
  If run as regular user (e.g. './install.sh'), user-only mode is selected by default.
EOF
}

MODE=""
ACTION="install"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --system)
            MODE="system"
            shift
            ;;
        --user)
            MODE="user"
            shift
            ;;
        -u|--uninstall)
            ACTION="uninstall"
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Auto-detect mode if not explicitly specified
if [[ -z "$MODE" ]]; then
    if [[ $EUID -eq 0 ]]; then
        MODE="system"
    else
        MODE="user"
    fi
fi

if [[ "$ACTION" == "uninstall" ]]; then
    echo "==> Uninstalling Zephys Terminal (Mode: $MODE)..."
    if [[ "$MODE" == "system" ]]; then
        if [[ $EUID -ne 0 ]]; then
            echo "Error: System-wide uninstallation requires root privileges. Please run with sudo:"
            echo "  sudo ./install.sh --uninstall --system"
            exit 1
        fi

        # Remove alternatives if registered
        if command -v update-alternatives >/dev/null 2>&1; then
            echo "--> Removing update-alternatives for x-terminal-emulator..."
            update-alternatives --remove x-terminal-emulator /usr/local/bin/zephys 2>/dev/null || true
        fi

        echo "--> Removing system binaries and integration files..."
        rm -f /usr/local/bin/zephys
        rm -f /usr/local/share/applications/zephys.desktop
        rm -f /usr/share/applications/zephys.desktop
        rm -f /usr/local/share/xfce4/helpers/zephys.desktop
        rm -f /usr/share/xfce4/helpers/zephys.desktop
        rm -f /usr/local/share/nemo/actions/zephys.nemo_action
        rm -f /usr/share/nemo/actions/zephys.nemo_action
        rm -f /usr/local/share/kio/servicemenus/zephys.desktop
        rm -f /usr/share/kio/servicemenus/zephys.desktop
        rm -f /usr/local/share/file-manager/actions/zephys.desktop
        rm -f /usr/share/file-manager/actions/zephys.desktop

        if command -v update-desktop-database >/dev/null 2>&1; then
            update-desktop-database /usr/local/share/applications 2>/dev/null || true
            update-desktop-database /usr/share/applications 2>/dev/null || true
        fi
        echo "==> System uninstallation complete!"
    else
        echo "--> Removing user binaries and integration files..."
        rm -f "$HOME/.local/bin/zephys"
        rm -f "$HOME/.local/share/applications/zephys.desktop"
        rm -f "$HOME/.local/share/xfce4/helpers/zephys.desktop"
        rm -f "$HOME/.local/share/nemo/actions/zephys.nemo_action"
        rm -f "$HOME/.local/share/kio/servicemenus/zephys.desktop"
        rm -f "$HOME/.local/share/nautilus/scripts/Open in Zephys"
        rm -f "$HOME/.local/share/file-manager/actions/zephys.desktop"

        if command -v update-desktop-database >/dev/null 2>&1; then
            update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
        fi
        echo "==> User uninstallation complete!"
    fi
    exit 0
fi

echo "==> Building latest Zephys Terminal..."
make clean
make

echo "==> Installing Zephys Terminal (Mode: $MODE)..."

if [[ "$MODE" == "system" ]]; then
    if [[ $EUID -ne 0 ]]; then
        echo "Error: System-wide installation requires root privileges. Please run with sudo:"
        echo "  sudo ./install.sh --system"
        exit 1
    fi

    # Binaries and desktop file
    install -d /usr/local/bin
    install -m 755 zephys /usr/local/bin/zephys
    install -d /usr/local/share/applications
    install -m 644 zephys.desktop /usr/local/share/applications/zephys.desktop

    # File manager and desktop integrations
    install -d /usr/share/xfce4/helpers
    install -m 644 integrations/xfce4-helper-zephys.desktop /usr/share/xfce4/helpers/zephys.desktop
    install -d /usr/local/share/xfce4/helpers
    install -m 644 integrations/xfce4-helper-zephys.desktop /usr/local/share/xfce4/helpers/zephys.desktop

    install -d /usr/share/nemo/actions
    install -m 644 integrations/nemo-action-zephys.nemo_action /usr/share/nemo/actions/zephys.nemo_action

    install -d /usr/share/kio/servicemenus
    install -m 644 integrations/dolphin-servicemenu-zephys.desktop /usr/share/kio/servicemenus/zephys.desktop

    install -d /usr/share/file-manager/actions
    install -m 644 integrations/pcmanfm-action-zephys.desktop /usr/share/file-manager/actions/zephys.desktop

    # Update alternatives for x-terminal-emulator
    if command -v update-alternatives >/dev/null 2>&1; then
        echo "--> Registering with update-alternatives as x-terminal-emulator (priority 50)..."
        update-alternatives --install /usr/bin/x-terminal-emulator x-terminal-emulator /usr/local/bin/zephys 50
    fi

    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database /usr/local/share/applications 2>/dev/null || true
    fi

    echo "==> Zephys successfully installed system-wide to /usr/local!"
else
    # User-only installation
    install -d "$HOME/.local/bin"
    install -m 755 zephys "$HOME/.local/bin/zephys"
    install -d "$HOME/.local/share/applications"
    install -m 644 zephys.desktop "$HOME/.local/share/applications/zephys.desktop"

    # User file manager integrations
    install -d "$HOME/.local/share/xfce4/helpers"
    install -m 644 integrations/xfce4-helper-zephys.desktop "$HOME/.local/share/xfce4/helpers/zephys.desktop"

    install -d "$HOME/.local/share/nemo/actions"
    install -m 644 integrations/nemo-action-zephys.nemo_action "$HOME/.local/share/nemo/actions/zephys.nemo_action"

    install -d "$HOME/.local/share/kio/servicemenus"
    install -m 644 integrations/dolphin-servicemenu-zephys.desktop "$HOME/.local/share/kio/servicemenus/zephys.desktop"

    install -d "$HOME/.local/share/nautilus/scripts"
    install -m 755 integrations/nautilus-script-zephys "$HOME/.local/share/nautilus/scripts/Open in Zephys"

    install -d "$HOME/.local/share/file-manager/actions"
    install -m 644 integrations/pcmanfm-action-zephys.desktop "$HOME/.local/share/file-manager/actions/zephys.desktop"

    # Configure XFCE helper preference for current user
    mkdir -p "$HOME/.config/xfce4"
    if [[ -f "$HOME/.config/xfce4/helpers.rc" ]]; then
        if grep -q '^TerminalEmulator=' "$HOME/.config/xfce4/helpers.rc"; then
            sed -i 's/^TerminalEmulator=.*/TerminalEmulator=zephys/' "$HOME/.config/xfce4/helpers.rc"
        else
            echo 'TerminalEmulator=zephys' >> "$HOME/.config/xfce4/helpers.rc"
        fi
    else
        echo 'TerminalEmulator=zephys' > "$HOME/.config/xfce4/helpers.rc"
    fi

    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
    fi

    echo "==> Zephys successfully installed for user $(whoami) in $HOME/.local!"
fi
