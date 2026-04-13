#!/usr/bin/env bash
# netns_setup.sh — Crée N namespaces réseau isolés reliés par un bridge virtuel
# Usage : sudo ./netns_setup.sh <nombre_de_machines>
#         sudo ./netns_setup.sh clean          → supprime tout
 
set -euo pipefail
 
# ─── Couleurs ────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'
 
# ─── Config ──────────────────────────────────────────────────────────────────
BRIDGE="br-netns"
SUBNET="10.0.0"
PORT=4444
 
# ─── Helpers ─────────────────────────────────────────────────────────────────
die()  { echo -e "${RED}[ERREUR]${RESET} $*" >&2; exit 1; }
info() { echo -e "${CYAN}[INFO]${RESET}  $*"; }
ok()   { echo -e "${GREEN}[OK]${RESET}    $*"; }
 
check_root() {
    [[ $EUID -eq 0 ]] || die "Ce script doit être exécuté en root (sudo)."
}
 
# ─── Nettoyage ───────────────────────────────────────────────────────────────
clean() {
    info "Suppression des namespaces et interfaces existants..."
    # Supprimer tous les namespaces netns-N
    for ns in $(ip netns list 2>/dev/null | awk '{print $1}' | grep '^netns-[0-9]'); do
        ip netns del "$ns" 2>/dev/null && ok "Namespace $ns supprimé"
    done
    # Supprimer le bridge
    if ip link show "$BRIDGE" &>/dev/null; then
        ip link set "$BRIDGE" down 2>/dev/null
        ip link del "$BRIDGE" 2>/dev/null && ok "Bridge $BRIDGE supprimé"
    fi
    # Supprimer les veth orphelins
    for v in $(ip link show 2>/dev/null | grep -oP 'veth-br\d+' || true); do
        ip link del "$v" 2>/dev/null && ok "Interface $v supprimée"
    done
    ok "Nettoyage terminé."
    exit 0
}
 
# ─── Création du bridge ──────────────────────────────────────────────────────
setup_bridge() {
    if ip link show "$BRIDGE" &>/dev/null; then
        info "Bridge $BRIDGE déjà présent, réutilisation."
    else
        ip link add "$BRIDGE" type bridge
        ip link set "$BRIDGE" up
        ok "Bridge $BRIDGE créé"
    fi
}
 
# ─── Création d'un namespace ─────────────────────────────────────────────────
setup_ns() {
    local idx=$1
    local ns="netns-${idx}"
    local ip_addr="${SUBNET}.${idx}"
    local veth_host="veth-br${idx}"
    local veth_ns="veth${idx}"
 
    # Namespace
    if ip netns list | grep -q "^${ns}"; then
        info "Namespace $ns déjà existant, passage..."
        return
    fi
 
    ip netns add "$ns"
 
    # Paire veth
    ip link add "$veth_host" type veth peer name "$veth_ns"
 
    # Brancher l'extrémité hôte au bridge
    ip link set "$veth_host" master "$BRIDGE"
    ip link set "$veth_host" up
 
    # Déplacer l'autre extrémité dans le namespace
    ip link set "$veth_ns" netns "$ns"
 
    # Configurer IP + loopback dans le namespace
    ip netns exec "$ns" ip addr add "${ip_addr}/24" dev "$veth_ns"
    ip netns exec "$ns" ip link set "$veth_ns" up
    ip netns exec "$ns" ip link set lo up
 
    ok "Namespace $ns  →  interface: ${BOLD}${veth_ns}${RESET}  IP: ${BOLD}${ip_addr}${RESET}"
}
 
# ─── Résumé final ────────────────────────────────────────────────────────────
print_summary() {
    local n=$1
    echo ""
    echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo -e "${BOLD}  Résumé — $n machine(s) prête(s)${RESET}"
    echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    printf "  %-12s %-14s %-18s %s\n" "Namespace" "Interface" "IP" 
    echo "  ─────────────────────────────────────────────────────"
    for i in $(seq 1 "$n"); do
        local ns="netns-${i}"
        local iface="veth${i}"
        local ip="${SUBNET}.${i}"
        printf "  %-12s %-14s %-18s %s\n" \
            "$ns" "$iface" "$ip" 
    done
    echo ""
    echo -e "  ${YELLOW}Exemple d'utilisation :${RESET}"
    echo -e "  sudo ip netns exec netns-1 ./build/driver/driver ${SUBNET}.1"
    echo -e "  sudo ip netns exec netns-1 bash -c 'DRIVER_SOCKET_PATH=/tmp/driver_1.sock ./build/driver/driver ${SUBNET}.1'"
    echo ""
    echo -e "  ${YELLOW}Capturer le trafic :${RESET}"
    echo -e "  sudo ip netns exec netns-1 tcpdump -i veth1 port $PORT"
    echo ""
    echo -e "  ${YELLOW}Nettoyage :${RESET}"
    echo -e "  sudo $0 clean"
    echo -e "${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
}
 
# ─── Machine parseable (pour scripts) ────────────────────────────────────────
print_machine_info() {
    # Sortie parseable sur stderr pour scripts parents :
    # FORMAT: NS=netns-1 IFACE=veth1 IP=10.0.0.1
    for i in $(seq 1 "$1"); do
        echo "NS=netns-${i} IFACE=veth${i} IP=${SUBNET}.${i}" >&2
    done
}
 
# ─── Validation arg ──────────────────────────────────────────────────────────
validate_count() {
    local n=$1
    [[ "$n" =~ ^[1-9][0-9]*$ ]]       || die "Argument invalide : '$n'. Attendu : entier >= 1."
    [[ "$n" -le 253 ]]                 || die "Maximum 253 machines (subnet /24)."
}
 
# ─── Main ────────────────────────────────────────────────────────────────────
main() {
    [[ $# -ge 1 ]] || { echo -e "Usage: sudo $0 <nombre_machines> | clean"; exit 1; }
 
    check_root
 
    [[ "$1" == "clean" ]] && clean
 
    local N=$1
    validate_count "$N"
 
    info "Création de $N namespace(s) réseau..."
    setup_bridge
 
    for i in $(seq 1 "$N"); do
        setup_ns "$i"
    done
 
    print_summary "$N"
    print_machine_info "$N"
}
 
main "$@"
