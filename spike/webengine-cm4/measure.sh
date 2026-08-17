#!/bin/sh
# Phase 0 spike — measure QtWebEngine's real memory footprint on CM4.
#
# Chromium is multi-process: the browser process forks a zygote, a GPU process,
# and one renderer per site. Reading the main process's RSS therefore tells you
# almost nothing. This samples the whole process tree.
#
# Pss (proportional set size) is used where the kernel exposes it, because
# summing RSS across Chromium processes double-counts the shared mappings and
# can overstate the total by hundreds of MB. Falls back to RSS if smaps_rollup
# is unreadable.
#
# usage: ./measure.sh <url> [extra args passed to the spike binary]

set -e

BIN="${BIN:-./webengine-spike}"
INTERVAL="${INTERVAL:-0.5}"

if [ -z "$1" ]; then
    echo "usage: $0 <url> [spike args...]" >&2
    exit 2
fi

if [ ! -x "$BIN" ]; then
    echo "$BIN not found or not executable — build it first (see README.md)" >&2
    exit 2
fi

# All descendants of $1, breadth-first. No pgrep dependency: Raspberry Pi OS
# has it, but a busybox-ish rescue shell might not.
tree_pids() {
    _pids="$1"
    _frontier="$1"
    while [ -n "$_frontier" ]; do
        _next=""
        for _p in $_frontier; do
            _kids=$(ps -o pid= --ppid "$_p" 2>/dev/null || true)
            for _k in $_kids; do
                _next="$_next $_k"
            done
        done
        _frontier="$_next"
        _pids="$_pids $_next"
    done
    echo "$_pids"
}

# Total memory of a pid list, in kB. Prefers Pss, falls back to RSS per process.
tree_kb() {
    _total=0
    for _p in $1; do
        _kb=$(awk '/^Pss:/ {s+=$2} END {if (s>0) print s}' \
              "/proc/$_p/smaps_rollup" 2>/dev/null || true)
        if [ -z "$_kb" ]; then
            _kb=$(awk '/^VmRSS:/ {print $2}' "/proc/$_p/status" 2>/dev/null || true)
        fi
        [ -n "$_kb" ] && _total=$((_total + _kb))
    done
    echo "$_total"
}

if [ -r "/proc/self/smaps_rollup" ]; then
    METRIC="Pss"
else
    METRIC="RSS (shared pages double-counted — treat as an upper bound)"
fi

echo "=== QtWebEngine CM4 spike ==="
echo "binary:   $BIN"
echo "metric:   $METRIC"
echo "sampling: every ${INTERVAL}s"
echo

MEMTOTAL=$(awk '/^MemTotal:/ {printf "%.0f", $2/1024}' /proc/meminfo)
echo "device RAM: ${MEMTOTAL} MB"
echo

# shellcheck disable=SC2086
"$BIN" "$@" &
APP_PID=$!

PEAK=0
PEAK_PROCS=0
SAMPLES=0
SUM=0

while kill -0 "$APP_PID" 2>/dev/null; do
    PIDS=$(tree_pids "$APP_PID")
    KB=$(tree_kb "$PIDS")
    NPROC=$(echo "$PIDS" | wc -w)
    if [ "$KB" -gt 0 ]; then
        SAMPLES=$((SAMPLES + 1))
        SUM=$((SUM + KB))
        if [ "$KB" -gt "$PEAK" ]; then
            PEAK=$KB
            PEAK_PROCS=$NPROC
        fi
    fi
    sleep "$INTERVAL"
done

wait "$APP_PID" 2>/dev/null || true

echo
echo "=== memory ==="
if [ "$SAMPLES" -gt 0 ]; then
    awk -v peak="$PEAK" -v procs="$PEAK_PROCS" -v sum="$SUM" \
        -v n="$SAMPLES" -v total="$MEMTOTAL" 'BEGIN {
        printf "peak:      %.1f MB across %d processes\n", peak/1024, procs
        printf "mean:      %.1f MB over %d samples\n", sum/n/1024, n
        printf "headroom:  %.1f MB of %d MB left at peak\n", total - peak/1024, total
    }'
else
    echo "no samples collected — did the binary exit immediately?"
fi
