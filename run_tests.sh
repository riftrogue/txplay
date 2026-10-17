#!/usr/bin/env bash
# run_tests.sh — Build Txplay and run every automated test runner sequentially.
#
# Usage:
#   ./run_tests.sh              # build + test
#   ./run_tests.sh --no-build   # skip cmake build, just run tests
#
# Exit codes:
#   0 — all tests passed
#   1 — build failed or at least one test runner failed
#
# Tests are run in dependency order (simpler / faster runners first).
# The script stops immediately on the first failure and reports which
# runner failed. audio_test_runner is intentionally excluded: it requires
# a user-supplied audio file path and interactive playback.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# ---------------------------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------------------------
BUILD=true
for arg in "$@"; do
    case "$arg" in
        --no-build) BUILD=false ;;
        *) echo "Unknown argument: $arg" >&2; exit 1 ;;
    esac
done

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
if $BUILD; then
    echo "==> Building..."
    cmake --build "$BUILD_DIR" 2>&1
    echo ""
fi

# ---------------------------------------------------------------------------
# Test runners in order.
# Each entry is: <executable> <display name>
# ---------------------------------------------------------------------------
declare -a RUNNERS=(
    "ui_util_test_runner"
    "input_adapter_test_runner"
    "config_test_runner"
    "library_test_runner"
    "visualizer_test_runner"
    "fft_test_runner"
    "application_test_runner"
)

PASS=0
FAIL=0

for runner in "${RUNNERS[@]}"; do
    exe="$BUILD_DIR/$runner"
    if [[ ! -x "$exe" ]]; then
        echo "ERROR: $runner not found or not executable at $exe" >&2
        exit 1
    fi
    echo "=== $runner ==="
    # Run from project root so relative fixture paths (tests/fixtures/, tests/tmp/) resolve correctly.
    if (cd "$SCRIPT_DIR" && "$exe"); then
        PASS=$(( PASS + 1 ))
    else
        FAIL=$(( FAIL + 1 ))
        echo ""
        echo "FAILED: $runner exited with non-zero status." >&2
        echo "Stopping. ($PASS passed before this failure.)" >&2
        exit 1
    fi
    echo ""
done

echo "=============================="
echo "All $PASS test runners passed."
echo "=============================="
