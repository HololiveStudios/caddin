#!/usr/bin/env bash
# ==============================================================================
# Caddin - Virtual Pet SNN (Instalador Linux)
# Versão: 1.0.2
# ==============================================================================

set -e

COLOR_RESET="\033[0m"
COLOR_BOLD="\033[1m"
COLOR_CYAN="\033[36m"
COLOR_GREEN="\033[32m"
COLOR_YELLOW="\033[33m"
COLOR_RED="\033[31m"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_SOURCE="${SCRIPT_DIR}/build/caddin"
if [ ! -f "${BIN_SOURCE}" ] && [ -f "${SCRIPT_DIR}/bin/caddin" ]; then
    BIN_SOURCE="${SCRIPT_DIR}/bin/caddin"
fi

show_banner() {
    echo -e "${COLOR_CYAN}${COLOR_BOLD}"
    echo "  ╔══════════════════════════════════════════════════════╗"
    echo "  ║            CADDIN - NEURAL VIRTUAL PET               ║"
    echo "  ║        Simulador SNN 1024 Neurônios • v1.0.2         ║"
    echo "  ╚══════════════════════════════════════════════════════╝"
    echo -e "${COLOR_RESET}"
}

check_dependencies() {
    echo -e "${COLOR_YELLOW}[1/4] Verificando dependências do sistema...${COLOR_RESET}"
    local missing=0

    if ! ldconfig -p 2>/dev/null | grep -q "libX11.so" && [ ! -f /usr/lib/x86_64-linux-gnu/libX11.so.6 ]; then
        echo -e "${COLOR_RED}  ✗ libX11 não encontrada.${COLOR_RESET}"
        missing=1
    fi

    if ! ldconfig -p 2>/dev/null | grep -q "libXext.so" && [ ! -f /usr/lib/x86_64-linux-gnu/libXext.so.6 ]; then
        echo -e "${COLOR_RED}  ✗ libXext não encontrada.${COLOR_RESET}"
        missing=1
    fi

    if [ $missing -eq 1 ]; then
        echo -e "${COLOR_YELLOW}Dica de instalação das bibliotecas X11:${COLOR_RESET}"
        echo "  Debian/Ubuntu: sudo apt install libx11-6 libxext6"
        echo "  Fedora:        sudo dnf install libX11 libXext"
        echo "  Arch Linux:    sudo pacman -S libx11 libxext"
    else
        echo -e "${COLOR_GREEN}  ✓ Todas as dependências básicas X11 foram encontradas.${COLOR_RESET}"
    fi
}

build_if_needed() {
    echo -e "${COLOR_YELLOW}[2/4] Verificando executável compilado...${COLOR_RESET}"
    if [ ! -f "${BIN_SOURCE}" ]; then
        echo -e "  Compilando binário nativo para Linux..."
        mkdir -p "${SCRIPT_DIR}/build"
        cd "${SCRIPT_DIR}/build"
        cmake .. -DCMAKE_BUILD_TYPE=Release
        cmake --build . -j"$(nproc 2>/dev/null || echo 2)"
        cd "${SCRIPT_DIR}"
        BIN_SOURCE="${SCRIPT_DIR}/build/caddin"
    fi
    echo -e "${COLOR_GREEN}  ✓ Executável pronto: ${BIN_SOURCE}${COLOR_RESET}"
}

install_caddin() {
    echo -e "${COLOR_YELLOW}[3/4] Instalando Caddin no sistema...${COLOR_RESET}"

    local TARGET_DIR="/usr/local/bin"
    local USE_SUDO=""

    if [ "$EUID" -ne 0 ]; then
        if command -v sudo >/dev/null 2>&1; then
            echo -e "  Requer privilégios para instalar em ${TARGET_DIR}. Solicitando sudo..."
            USE_SUDO="sudo"
        else
            TARGET_DIR="${HOME}/.local/bin"
            echo -e "  Sem sudo; instalando em ${TARGET_DIR}..."
            mkdir -p "${TARGET_DIR}"
        fi
    fi

    ${USE_SUDO} install -m 755 "${BIN_SOURCE}" "${TARGET_DIR}/caddin"
    echo -e "${COLOR_GREEN}  ✓ Binário instalado em: ${TARGET_DIR}/caddin${COLOR_RESET}"

    # Criação do lançador desktop no menu de aplicativos do Linux
    echo -e "${COLOR_YELLOW}[4/4] Criando atalho no menu de aplicativos (.desktop)...${COLOR_RESET}"
    local APPS_DIR="${HOME}/.local/share/applications"
    mkdir -p "${APPS_DIR}"

    cat <<EOF > "${APPS_DIR}/caddin.desktop"
[Desktop Entry]
Type=Application
Name=Caddin
GenericName=Virtual Pet SNN
Comment=Mascote virtual autônomo com cérebro de 1024 neurônios pulsados (SNN)
Exec=${TARGET_DIR}/caddin
Terminal=false
Categories=Game;Simulation;Utility;
Keywords=pet;virtual;ai;snn;neural;
StartupNotify=true
EOF

    chmod +x "${APPS_DIR}/caddin.desktop"
    echo -e "${COLOR_GREEN}  ✓ Atalho criado em: ${APPS_DIR}/caddin.desktop${COLOR_RESET}"

    echo ""
    echo -e "${COLOR_GREEN}${COLOR_BOLD}Instalação concluída com sucesso!${COLOR_RESET}"
    echo -e "Você pode iniciar o Caddin digitando ${COLOR_CYAN}caddin${COLOR_RESET} no terminal ou pelo menu de aplicativos."
}

uninstall_caddin() {
    echo -e "${COLOR_YELLOW}Removendo Caddin do sistema...${COLOR_RESET}"
    local TARGET_DIR="/usr/local/bin"
    if [ -f "${TARGET_DIR}/caddin" ]; then
        if [ "$EUID" -ne 0 ] && command -v sudo >/dev/null 2>&1; then
            sudo rm -f "${TARGET_DIR}/caddin"
        else
            rm -f "${TARGET_DIR}/caddin"
        fi
        echo -e "${COLOR_GREEN}  ✓ Removido de ${TARGET_DIR}/caddin${COLOR_RESET}"
    fi

    if [ -f "${HOME}/.local/bin/caddin" ]; then
        rm -f "${HOME}/.local/bin/caddin"
        echo -e "${COLOR_GREEN}  ✓ Removido de ~/.local/bin/caddin${COLOR_RESET}"
    fi

    if [ -f "${HOME}/.local/share/applications/caddin.desktop" ]; then
        rm -f "${HOME}/.local/share/applications/caddin.desktop"
        echo -e "${COLOR_GREEN}  ✓ Removido atalho .desktop${COLOR_RESET}"
    fi
    echo -e "${COLOR_GREEN}Desinstalação concluída!${COLOR_RESET}"
}

if [ "$1" = "--uninstall" ] || [ "$1" = "-u" ]; then
    show_banner
    uninstall_caddin
    exit 0
fi

show_banner
check_dependencies
build_if_needed
install_caddin
