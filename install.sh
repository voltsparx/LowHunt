#!/usr/bin/env bash
set -euo pipefail

TOOL="lowhunt"
INSTALL_DATA="/usr/share/lowhunt"
CYAN='\033[1;36m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
RESET='\033[0m'

echo -e "${CYAN}[*] Building ${TOOL}...${RESET}"
make clean
make -j"$(nproc)"

echo -e "${CYAN}[*] Installing binary and data files...${RESET}"
sudo make install

echo -e "${GREEN}[+] Installed ${TOOL}.${RESET}"
echo -e "${YELLOW}    Data directory: ${INSTALL_DATA}${RESET}"
echo -e "${GREEN}    Try: lowhunt --help${RESET}"
