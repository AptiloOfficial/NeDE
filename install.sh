#!/bin/bash
#
# NeDE — Efficient Desktop Environment
# Universal Installer for Linux
#
# Version: 0.1.0
# License: MIT
#

set -e  # Выход при ошибке

# ============ ЦВЕТА ============
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# ============ ЛОГОТИП ============
print_logo() {
    echo -e "${CYAN}"
    cat << "EOF"
    _   _      ____  _____ 
   | \ | |    |  _ \| ____|
   |  \| | ___| | | |  _|  
   | |\  |/ _ \ |_| | |___ 
   |_| \_|\___/____/|_____|
   
   Efficient Desktop Environment v0.1.0
EOF
    echo -e "${NC}"
}

# ============ ЛОГИ ============
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_ok()   { echo -e "${GREEN}[ OK ]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_err()  { echo -e "${RED}[FAIL]${NC} $1"; }

# ============ ОПРЕДЕЛЕНИЕ ДИСТРИБУТИВА ============
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO="$ID"
        DISTRO_NAME="$PRETTY_NAME"
    elif [ -f /etc/lsb-release ]; then
        . /etc/lsb-release
        DISTRO="$DISTRIB_ID"
        DISTRO_NAME="$DISTRIB_DESCRIPTION"
    else
        DISTRO="unknown"
        DISTRO_NAME="Unknown Linux"
    fi
    
    log_info "Обнаружен дистрибутив: ${DISTRO_NAME}"
    log_info "ID: ${DISTRO}"
}

# ============ УСТАНОВКА ЗАВИСИМОСТЕЙ ============
install_deps_apt() {
    log_info "Установка зависимостей (apt)..."
    sudo apt update
    sudo apt install -y --no-install-recommends \
        build-essential pkg-config \
        libgtk-3-dev libgtk-layer-shell-dev \
        libpolkit-gobject-1-dev libpolkit-agent-1-dev libpam0g-dev \
        lua5.3 lua-lgi lua-filesystem \
        lightdm lightdm-gtk-greeter \
        labwc \
        pipewire pipewire-pulse wireplumber rtkit \
        network-manager \
        grim slurp wl-clipboard wlrctl \
        swaybg \
        fonts-dejavu-core
    log_ok "Зависимости установлены"
}

install_deps_pacman() {
    log_info "Установка зависимостей (pacman)..."
    sudo pacman -S --needed --noconfirm \
        base-devel pkgconf \
        gtk3 gtk-layer-shell \
        polkit lua lua-lgi lua-filesystem \
        lightdm lightdm-gtk-greeter \
        labwc \
        pipewire pipewire-pulse wireplumber rtkit \
        networkmanager \
        grim slurp wl-clipboard wlrctl \
        swaybg \
        ttf-dejavu
    log_ok "Зависимости установлены"
}

install_deps_dnf() {
    log_info "Установка зависимостей (dnf)..."
    sudo dnf install -y \
        gcc make pkgconfig \
        gtk3-devel gtk-layer-shell-devel \
        polkit-devel pam-devel \
        lua lua-lgi lua-filesystem \
        lightdm lightdm-gtk-greeter \
        labwc \
        pipewire pipewire-pulseaudio wireplumber rtkit \
        NetworkManager \
        grim slurp wl-clipboard wlrctl \
        swaybg \
        dejavu-sans-fonts
    log_ok "Зависимости установлены"
}

install_deps_zypper() {
    log_info "Установка зависимостей (zypper)..."
    sudo zypper install -y \
        gcc make pkg-config \
        gtk3-devel gtk-layer-shell-devel \
        polkit-devel pam-devel \
        lua53 lua53-lgi lua53-filesystem \
        lightdm lightdm-gtk-greeter \
        labwc \
        pipewire pipewire-pulseaudio wireplumber rtkit \
        NetworkManager \
        grim slurp wl-clipboard wlrctl \
        swaybg \
        dejavu-fonts
    log_ok "Зависимости установлены"
}

install_deps_apk() {
    log_info "Установка зависимостей (apk)..."
    sudo apk add \
        build-base pkgconf \
        gtk+3.0-dev gtk-layer-shell-dev \
        polkit-dev pam-dev \
        lua5.3 lua-lgi lua-filesystem \
        lightdm lightdm-gtk-greeter \
        labwc \
        pipewire pipewire-pulse wireplumber rtkit \
        networkmanager \
        grim slurp wl-clipboard wlrctl \
        swaybg \
        font-dejavu
    log_ok "Зависимости установлены"
}

install_deps() {
    case "$DISTRO" in
        debian|ubuntu|linuxmint|pop|kali|raspbian)
            install_deps_apt
            ;;
        arch|manjaro|endeavouros|garuda|artix)
            install_deps_pacman
            ;;
        fedora|rhel|centos|rocky|almalinux)
            install_deps_dnf
            ;;
        opensuse*|suse|sles)
            install_deps_zypper
            ;;
        alpine)
            install_deps_apk
            ;;
        *)
            log_warn "Неизвестный дистрибутив: $DISTRO"
            log_warn "Установите зависимости вручную:"
            echo ""
            echo "  - GTK 3.24+"
            echo "  - gtk-layer-shell"
            echo "  - polkit + pam"
            echo "  - lua5.3 + lua-lgi + lua-filesystem"
            echo "  - lightdm"
            echo "  - labwc"
            echo "  - pipewire + wireplumber"
            echo "  - network-manager"
            echo "  - grim + slurp + wl-clipboard + wlrctl"
            echo "  - swaybg"
            echo "  - fonts-dejavu-core"
            echo ""
            read -p "Продолжить установку? [y/N] " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
            ;;
    esac
}

# ============ ПРОВЕРКА ЗАВИСИМОСТЕЙ ============
check_deps() {
    log_info "Проверка зависимостей..."
    
    local missing=0
    
    # Проверка компиляторов
    command -v gcc >/dev/null 2>&1 || { log_err "gcc не найден"; missing=1; }
    command -v pkg-config >/dev/null 2>&1 || { log_err "pkg-config не найден"; missing=1; }
    
    # Проверка библиотек
    pkg-config --exists gtk+-3.0 || { log_err "GTK3 не найден"; missing=1; }
    pkg-config --exists gtk-layer-shell-0 || { log_err "gtk-layer-shell не найден"; missing=1; }
    pkg-config --exists polkit-gobject-1 || { log_err "polkit не найден"; missing=1; }
    
    # Проверка Lua
    command -v lua >/dev/null 2>&1 || { log_err "lua не найден"; missing=1; }
    lua -e "require('lgi')" 2>/dev/null || { log_err "lua-lgi не найден"; missing=1; }
    
    if [ $missing -eq 1 ]; then
        log_err "Не все зависимости установлены!"
        return 1
    fi
    
    log_ok "Все зависимости на месте"
    return 0
}

# ============ СБОРКА ============
build_nede() {
    log_info "Сборка NeDE..."
    
    if [ ! -f "Makefile" ]; then
        log_err "Makefile не найден. Запустите скрипт из корня NeDE."
        exit 1
    fi
    
    make clean >/dev/null 2>&1 || true
    make all 2>&1 | grep -E "Building|OK|error" || true
    
    if [ ! -f "panel/nedepanel" ]; then
        log_err "Сборка не удалась!"
        exit 1
    fi
    
    log_ok "Сборка завершена"
}

# ============ УСТАНОВКА ============
install_nede() {
    log_info "Установка NeDE в /usr/local..."
    
    # Бинарники
    sudo install -Dm755 panel/nedepanel         /usr/local/bin/nedepanel
    sudo install -Dm755 desktop/nedesktop       /usr/local/bin/nedesktop
    sudo install -Dm755 nedeshot/nedess         /usr/local/bin/nedess
    sudo install -Dm755 nedeuac/nedeuac         /usr/local/bin/nedeuac
    sudo install -Dm755 launcher/nedelauncher   /usr/local/bin/nedelauncher
    
    # Wayland session
    sudo mkdir -p /usr/share/wayland-sessions
    sudo cat > /usr/share/wayland-sessions/nede.desktop << 'EOFDESKTOP'
[Desktop Entry]
Name=NeDE
Comment=NeDE Efficient Desktop Environment
Exec=env XDG_CURRENT_DESKTOP=NeDE labwc
Type=Application
EOFDESKTOP
    
    # PAM config
    if [ ! -f /etc/pam.d/polkit-1 ]; then
        sudo cat > /etc/pam.d/polkit-1 << 'EOFPAM'
#%PAM-1.0
auth       required   pam_env.so readenv=1 user_readenv=0
auth       required   pam_env.so readenv=1 envfile=/etc/default/locale user_readenv=0
@include common-auth
@include common-account
@include common-session-noninteractive
EOFPAM
        log_ok "PAM config установлен"
    else
        log_info "PAM config уже существует — пропускаем"
    fi
    
    # LabWC autostart (для текущего пользователя)
    mkdir -p ~/.config/labwc
    if [ ! -f ~/.config/labwc/autostart ]; then
        cat > ~/.config/labwc/autostart << 'EOFAUTO'
# NeDE autostart

# Обои
swaybg -i /usr/share/images/desktop-base/default -m fill &

# Панель
nedepanel &

# NeDE UAC (Polkit агент)
pkill -9 nedeuac 2>/dev/null
sleep 1
nohup /usr/local/bin/nedeuac >/dev/null 2>&1 &

# Рабочий стол
nedesktop &
EOFAUTO
        chmod +x ~/.config/labwc/autostart
        log_ok "LabWC autostart установлен"
    else
        log_info "LabWC autostart уже существует — пропускаем"
    fi
    
    log_ok "NeDE установлена"
}

# ============ АКТИВАЦИЯ ============
enable_services() {
    log_info "Активация сервисов..."
    
    # LightDM
    if systemctl list-unit-files 2>/dev/null | grep -q lightdm; then
        sudo systemctl enable lightdm 2>/dev/null || true
        log_ok "LightDM включён"
    fi
    
    # PipeWire (для текущего пользователя)
    systemctl --user enable pipewire pipewire-pulse wireplumber 2>/dev/null || true
    log_ok "PipeWire включён"
    
    # RTKit
    if systemctl list-unit-files 2>/dev/null | grep -q rtkit; then
        sudo systemctl enable --now rtkit-daemon 2>/dev/null || true
        log_ok "RTKit включён"
    fi
    
    # NetworkManager
    if systemctl list-unit-files 2>/dev/null | grep -q NetworkManager; then
        sudo systemctl enable --now NetworkManager 2>/dev/null || true
        log_ok "NetworkManager включён"
    fi
    
    # Polkit
    if systemctl list-unit-files 2>/dev/null | grep -q polkit; then
        sudo systemctl enable --now polkit 2>/dev/null || true
        log_ok "Polkit включён"
    fi
}

# ============ ФИНАЛ ============
print_summary() {
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║       NeDE v0.1.0 успешно установлена!        ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${CYAN}Установленные компоненты:${NC}"
    echo "  • nedepanel      — панель задач"
    echo "  • nedesktop      — рабочий стол"
    echo "  • nedess         — скриншотер"
    echo "  • nedeuac        — Polkit-агент"
    echo "  • nedelauncher   — лаунчер"
    echo ""
    echo -e "${CYAN}Что дальше:${NC}"
    echo "  1. Выйдите из текущей сессии"
    echo "  2. В LightDM выберите 'NeDE'"
    echo "  3. Войдите в систему"
    echo ""
    echo -e "${CYAN}Горячие клавиши (в NeDE):${NC}"
    echo "  • Super + Space   — лаунчер"
    echo "  • Super + Shift + S — скриншот"
    echo "  • Print          — скриншот"
    echo ""
    echo -e "${CYAN}Проверка:${NC}"
    echo "  which nedepanel nedesktop nedess nedeuac nedelauncher"
    echo ""
    echo -e "${GREEN}Спасибо за установку NeDE!${NC}"
    echo ""
}

# ============ УДАЛЕНИЕ ============
uninstall_nede() {
    log_info "Удаление NeDE..."
    
    sudo rm -f /usr/local/bin/nedepanel
    sudo rm -f /usr/local/bin/nedesktop
    sudo rm -f /usr/local/bin/nedess
    sudo rm -f /usr/local/bin/nedeuac
    sudo rm -f /usr/local/bin/nedelauncher
    sudo rm -f /usr/share/wayland-sessions/nede.desktop
    
    log_ok "NeDE удалена"
}

# ============ ПОМОЩЬ ============
show_help() {
    cat << "EOF"
NeDE Installer v0.1.0

Использование: ./install.sh [OPTION]

Опции:
  (без опций)      Установить NeDE
  --build-only     Только собрать, без установки
  --deps-only      Только установить зависимости
  --uninstall      Удалить NeDE
  --help           Показать эту справку

Примеры:
  ./install.sh              # Полная установка
  ./install.sh --build-only # Собрать без установки
  ./install.sh --uninstall  # Удалить NeDE
EOF
}

# ============ ГЛАВНАЯ ============
main() {
    print_logo
    
    case "${1:-}" in
        --help|-h)
            show_help
            exit 0
            ;;
        --uninstall)
            uninstall_nede
            exit 0
            ;;
        --build-only)
            detect_distro
            if ! check_deps; then
                log_err "Установите зависимости сначала: ./install.sh --deps-only"
                exit 1
            fi
            build_nede
            exit 0
            ;;
        --deps-only)
            detect_distro
            install_deps
            exit 0
            ;;
        "")
            # Полная установка
            detect_distro
            install_deps
            if ! check_deps; then
                log_err "Не все зависимости установлены"
                exit 1
            fi
            build_nede
            install_nede
            enable_services
            print_summary
            ;;
        *)
            log_err "Неизвестная опция: $1"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
