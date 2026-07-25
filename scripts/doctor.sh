#!/usr/bin/env bash
#
# Environment doctor for "Thinking in Cycles" (BLUEPRINT.md Section 13.3).
# Reports what this machine can and cannot support for the book's labs.
# Never exits non-zero: a missing tool or unsupported feature is reported
# as a WARN/SKIP/MISSING line, not a build failure (BLUEPRINT.md Section 21:
# "CI must not fail a build because a benchmark did not reach a performance
# threshold" -- the same "warnings, not silent failures" principle applies
# to environment capability, not just performance thresholds).
#
# Deliberately NOT `set -e`: a check that fails partway through must still
# emit a status line, not abort the whole script.
set -uo pipefail

# ---------------------------------------------------------------- output --

USE_COLOR=0
if [ -t 1 ] && command -v tput >/dev/null 2>&1 && [ "$(tput colors 2>/dev/null || echo 0)" -ge 8 ]; then
  USE_COLOR=1
fi

color() { # color <code> <text>
  if [ "$USE_COLOR" -eq 1 ]; then
    printf '\033[%sm%s\033[0m' "$1" "$2"
  else
    printf '%s' "$2"
  fi
}

declare -a SUMMARY_LINES=()
declare -i COUNT_OK=0 COUNT_WARN=0 COUNT_SKIP=0 COUNT_MISSING=0

status_line() { # status_line <STATUS> <message>
  local status="$1"; shift
  local msg="$*"
  local tag
  case "$status" in
    OK)      tag=$(color 32 "[OK]");       COUNT_OK+=1 ;;
    WARN)    tag=$(color 33 "[WARN]");     COUNT_WARN+=1 ;;
    SKIP)    tag=$(color 36 "[SKIP]");     COUNT_SKIP+=1 ;;
    MISSING) tag=$(color 31 "[MISSING]");  COUNT_MISSING+=1 ;;
    *)       tag="[$status]" ;;
  esac
  printf '  %s %s\n' "$tag" "$msg"
  SUMMARY_LINES+=("[$status] $msg")
}

section() {
  printf '\n== %s ==\n' "$1"
}

have() { command -v "$1" >/dev/null 2>&1; }

# ------------------------------------------------------------ OS detect --

OS_NAME="$(uname -s 2>/dev/null || echo unknown)"
IS_LINUX=0
IS_DARWIN=0
case "$OS_NAME" in
  Linux) IS_LINUX=1 ;;
  Darwin) IS_DARWIN=1 ;;
esac

printf 'cyclelab environment doctor\n'
printf 'Detected OS: %s\n' "$OS_NAME"

if [ "$IS_LINUX" -eq 0 ]; then
  printf '\n'
  status_line WARN "This machine is not Linux ($OS_NAME). Thinking in Cycles targets Linux; every Linux-specific check below will report [SKIP]. Chapters 1-5 (the currently drafted material) do not require Linux, but Chapters 6+ and most of labs/cyclelab's other modes will."
fi

# --------------------------------------------------- kernel / distro ---

section "Kernel and Distribution"
if [ "$IS_LINUX" -eq 1 ]; then
  KREL="$(uname -r 2>/dev/null || echo unknown)"
  status_line OK "kernel release: $KREL"
  if [ -f /etc/os-release ]; then
    # shellcheck disable=SC1091
    DISTRO="$(. /etc/os-release 2>/dev/null && echo "${PRETTY_NAME:-unknown}")"
    status_line OK "distribution: ${DISTRO:-unknown}"
  else
    status_line WARN "no /etc/os-release found; distribution unknown"
  fi
elif [ "$IS_DARWIN" -eq 1 ]; then
  SWVERS="$(sw_vers -productVersion 2>/dev/null || echo unknown)"
  status_line SKIP "not applicable on Darwin (macOS $SWVERS detected; no Linux kernel to report)"
else
  status_line SKIP "not applicable on $OS_NAME"
fi

# ------------------------------------------------------- CPU / arch ----

section "CPU and Architecture"
ARCH="$(uname -m 2>/dev/null || echo unknown)"
status_line OK "architecture: $ARCH"
if [ "$IS_LINUX" -eq 1 ]; then
  if [ -r /proc/cpuinfo ]; then
    MODEL="$(grep -m1 -E 'model name|Model' /proc/cpuinfo 2>/dev/null | sed 's/^[^:]*: //')"
    status_line OK "CPU model: ${MODEL:-unknown}"
  else
    status_line WARN "/proc/cpuinfo not readable"
  fi
elif [ "$IS_DARWIN" -eq 1 ]; then
  MODEL="$(sysctl -n machdep.cpu.brand_string 2>/dev/null || echo unknown)"
  status_line OK "CPU model: $MODEL"
else
  status_line WARN "CPU model detection not implemented for $OS_NAME"
fi

# ------------------------------------------------------------ topology --

section "CPU Topology"
if [ "$IS_LINUX" -eq 1 ]; then
  if have lscpu; then
    SOCKETS="$(lscpu | awk -F: '/^Socket\(s\)/ {gsub(/ /,"",$2); print $2}')"
    CORES_PER_SOCKET="$(lscpu | awk -F: '/^Core\(s\) per socket/ {gsub(/ /,"",$2); print $2}')"
    THREADS_PER_CORE="$(lscpu | awk -F: '/^Thread\(s\) per core/ {gsub(/ /,"",$2); print $2}')"
    status_line OK "sockets=${SOCKETS:-?} cores/socket=${CORES_PER_SOCKET:-?} threads/core=${THREADS_PER_CORE:-?}"
  else
    status_line MISSING "lscpu not found; install util-linux for topology details"
  fi
elif [ "$IS_DARWIN" -eq 1 ]; then
  PHYS="$(sysctl -n hw.physicalcpu 2>/dev/null || echo '?')"
  LOGI="$(sysctl -n hw.logicalcpu 2>/dev/null || echo '?')"
  status_line OK "physical cores=$PHYS logical cores=$LOGI (macOS reports no socket/SMT breakdown here)"
else
  status_line SKIP "not applicable on $OS_NAME"
fi

# ----------------------------------------------------------------- NUMA -

section "NUMA"
if [ "$IS_LINUX" -eq 1 ]; then
  if have numactl; then
    NODES="$(numactl --hardware 2>/dev/null | awk '/^available:/ {print $2}')"
    status_line OK "numactl available; ${NODES:-?} node(s) reported"
  else
    status_line MISSING "numactl not found; needed for Chapters 24-25"
  fi
else
  status_line SKIP "not applicable on $OS_NAME (no NUMA concept outside Linux here)"
fi

# --------------------------------------------------- frequency/governor -

section "Frequency / Governor"
if [ "$IS_LINUX" -eq 1 ]; then
  GOV_PATH="/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
  if [ -r "$GOV_PATH" ]; then
    status_line OK "cpu0 governor: $(cat "$GOV_PATH")"
  else
    status_line WARN "no cpufreq governor exposed (common in VMs/containers); frequency-scaling caveats in Chapter 4 still apply"
  fi
elif [ "$IS_DARWIN" -eq 1 ]; then
  status_line SKIP "not applicable on Darwin (no user-visible governor); Apple silicon manages frequency automatically"
else
  status_line SKIP "not applicable on $OS_NAME"
fi

# ---------------------------------------------------------------- perf --

section "perf"
if [ "$IS_LINUX" -eq 1 ]; then
  if have perf; then
    PV="$(perf --version 2>/dev/null || echo unknown)"
    status_line OK "$PV"
  else
    status_line MISSING "perf not found; required from Chapter 10 onward (install linux-tools matching this kernel)"
  fi
else
  status_line SKIP "not applicable on $OS_NAME (perf is Linux-only)"
fi

# --------------------------------------------------- perf_event_paranoid

section "perf_event_paranoid"
if [ "$IS_LINUX" -eq 1 ]; then
  PP_PATH="/proc/sys/kernel/perf_event_paranoid"
  if [ -r "$PP_PATH" ]; then
    PP="$(cat "$PP_PATH")"
    status_line OK "perf_event_paranoid=$PP (lower allows more unprivileged access; see Chapter 10)"
  else
    status_line WARN "$PP_PATH not readable"
  fi
else
  status_line SKIP "not applicable on $OS_NAME"
fi

# --------------------------------------------------------- kptr_restrict

section "kptr_restrict"
if [ "$IS_LINUX" -eq 1 ]; then
  KR_PATH="/proc/sys/kernel/kptr_restrict"
  if [ -r "$KR_PATH" ]; then
    status_line OK "kptr_restrict=$(cat "$KR_PATH") (affects kernel symbol resolution; see Chapter 13)"
  else
    status_line WARN "$KR_PATH not readable"
  fi
else
  status_line SKIP "not applicable on $OS_NAME"
fi

# ------------------------------------------------------------------ BTF -

section "BTF"
if [ "$IS_LINUX" -eq 1 ]; then
  if [ -r /sys/kernel/btf/vmlinux ]; then
    status_line OK "/sys/kernel/btf/vmlinux present (CO-RE eBPF programs supported, Chapter 27)"
  else
    status_line MISSING "no /sys/kernel/btf/vmlinux; CO-RE eBPF (Chapter 27) will not work on this kernel"
  fi
else
  status_line SKIP "not applicable on $OS_NAME"
fi

# ------------------------------------------------------- bpftrace / BCC -

section "bpftrace / BCC"
if [ "$IS_LINUX" -eq 1 ]; then
  if have bpftrace; then
    status_line OK "bpftrace: $(bpftrace --version 2>/dev/null || echo present)"
  else
    status_line MISSING "bpftrace not found; required for Chapter 28"
  fi
  if have python3 && python3 -c 'import bcc' >/dev/null 2>&1; then
    status_line OK "BCC python bindings importable"
  elif have /usr/sbin/execsnoop-bpfcc || have execsnoop-bpfcc; then
    status_line OK "BCC tools present on PATH"
  else
    status_line MISSING "BCC not found (python bindings or packaged tools); required for Chapter 28"
  fi
else
  status_line SKIP "not applicable on $OS_NAME (bpftrace/BCC are Linux-only)"
fi

# ------------------------------------------------------- FlameGraph ----

section "FlameGraph scripts"
FOUND_FG=0
for candidate in \
  "$HOME/FlameGraph" \
  "/opt/FlameGraph" \
  "$(pwd)/FlameGraph"
do
  if [ -x "$candidate/flamegraph.pl" ]; then
    status_line OK "found at $candidate"
    FOUND_FG=1
    break
  fi
done
if [ "$FOUND_FG" -eq 0 ]; then
  if have flamegraph.pl; then
    status_line OK "flamegraph.pl on PATH"
  else
    status_line MISSING "FlameGraph scripts not found; clone https://github.com/brendangregg/FlameGraph for Chapter 14"
  fi
fi

# ------------------------------------------------------ PMU events -----

section "Supported PMU events"
if [ "$IS_LINUX" -eq 1 ]; then
  if have perf; then
    N="$(perf list 2>/dev/null | grep -c '\[Hardware event\]' || true)"
    status_line OK "perf list reports ${N:-0} hardware events (exact set is CPU/kernel-specific, see Appendix C)"
  else
    status_line MISSING "perf not available, cannot enumerate PMU events"
  fi
else
  status_line SKIP "not applicable on $OS_NAME"
fi

# -------------------------------------------------------- virtualization

section "Virtualization"
if [ "$IS_LINUX" -eq 1 ]; then
  if have systemd-detect-virt; then
    V="$(systemd-detect-virt 2>/dev/null || echo none)"
    if [ "$V" = "none" ]; then
      status_line OK "systemd-detect-virt reports bare metal"
    else
      status_line WARN "systemd-detect-virt reports virtualization: $V (Section 13.2 -- PMU/NUMA/eBPF limitations may apply)"
    fi
  else
    status_line WARN "systemd-detect-virt not found; cannot determine virtualization status"
  fi
elif [ "$IS_DARWIN" -eq 1 ]; then
  HV="$(sysctl -n kern.hv_vmm_present 2>/dev/null || echo 0)"
  if [ "$HV" = "1" ]; then
    status_line WARN "kern.hv_vmm_present=1 (running under a hypervisor)"
  else
    status_line OK "no hypervisor indicator found (kern.hv_vmm_present=0 or absent)"
  fi
else
  status_line SKIP "not applicable on $OS_NAME"
fi

# --------------------------------------------------- Intel PCM (optional)

section "Intel PCM (optional)"
if have pcm || have pcm.x; then
  status_line OK "Intel PCM found on PATH"
else
  status_line SKIP "Intel PCM not found (optional; only used in Chapter 20 uncore sidebars)"
fi

# ------------------------------------------------------------- summary --

section "Summary"
printf '  %s ok   %s warn   %s skip   %s missing\n' \
  "$COUNT_OK" "$COUNT_WARN" "$COUNT_SKIP" "$COUNT_MISSING"
if [ "$COUNT_MISSING" -gt 0 ] || [ "$COUNT_WARN" -gt 0 ]; then
  printf '\nThis is informational: doctor never fails the build (BLUEPRINT.md Section 21).\n'
  printf 'Missing/warned items only limit which labs you can run end-to-end.\n'
fi

exit 0
