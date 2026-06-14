#!/bin/bash
# DasherCore Linux diagnostic script
# Run in WSL2 to find leaks, hangs, and crashes
#
# Usage:
#   git clone https://github.com/dasher-project/DasherCore.git
#   cd DasherCore && git checkout feature-CAPI
#   git submodule update --init --recursive
#   bash ../debug-linux.sh
#
# Output: debug-output/ directory with results

set -e

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="$REPO_DIR/debug-output"
mkdir -p "$OUTPUT_DIR"

echo "========================================"
echo " DasherCore Linux Diagnostics"
echo "========================================"
echo ""

# ── 1. Install deps ──────────────────────────────────────────
echo "[1/5] Installing build dependencies..."
sudo apt-get update -qq
sudo apt-get install -y -qq build-essential cmake clang clang-tidy valgrind

# ── 2. Build (normal + sanitizer) ────────────────────────────
echo ""
echo "[2/5] Building DasherCore + tests..."

cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_CAPI=ON -S . 2>&1 | tail -3
cmake --build build -j$(nproc) 2>&1 | tail -3

echo ""
echo "  Building sanitizer version..."
cmake -B build-san -DCMAKE_BUILD_TYPE=Debug -DDASHER_SANITIZE=ON -DBUILD_TESTS=ON -S . 2>&1 | tail -3
cmake --build build-san -j$(nproc) 2>&1 | tail -3

# ── 3. Run tests (normal build) ──────────────────────────────
echo ""
echo "[3/5] Running tests (normal build)..."
cd build
ctest --output-on-failure --timeout 120 2>&1 | tee "$OUTPUT_DIR/tests-normal.txt"
cd ..

# ── 4. Find memory leaks ─────────────────────────────────────
echo ""
echo "[4/5] Hunting memory leaks..."
echo ""
echo "  --- ASan leak report: capi_tests ---"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=0:print_stacktrace=1 \
    ./build-san/dasher_capi_tests 2>&1 | tee "$OUTPUT_DIR/leaks-asan-capi.txt" || true

echo ""
echo "  --- ASan leak report: multilingual_tests ---"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=0:print_stacktrace=1 \
    ./build-san/dasher_multilingual_tests 2>&1 | tee "$OUTPUT_DIR/leaks-asan-multilingual.txt" || true

echo ""
echo "  --- ASan leak report: lifecycle_tests ---"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=0:print_stacktrace=1 \
    ./build-san/dasher_lifecycle_tests 2>&1 | tee "$OUTPUT_DIR/leaks-asan-lifecycle.txt" || true

echo ""
echo "  --- Valgrind: capi_tests (slower, more detail) ---"
valgrind --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=definite \
    ./build/dasher_capi_tests 2>&1 | tee "$OUTPUT_DIR/leaks-valgrind-capi.txt" || true

# ── 5. Debug LM hang + multilingual SEGV ─────────────────────
echo ""
echo "[5/5] Debugging LM hang and multilingual crash..."
echo ""
echo "  --- LM test (30s timeout) ---"
timeout 30 ./build/dasher_language_model_tests 2>&1 | tee "$OUTPUT_DIR/lm-test-output.txt" || true
echo ""
echo "  LM test exit code: $?"

echo ""
echo "  --- Multilingual under ASan ---"
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1:print_stacktrace=1 \
    ./build-san/dasher_multilingual_tests 2>&1 | tee "$OUTPUT_DIR/multilingual-asan.txt" || true

# ── Summary ──────────────────────────────────────────────────
echo ""
echo "========================================"
echo " Done. Results in debug-output/"
echo "========================================"
echo ""
echo "Files:"
ls -la "$OUTPUT_DIR/"
echo ""
echo "Key things to look for:"
echo "  - leaks-asan-*.txt:     'ERROR: LeakSanitizer' sections with stack traces"
echo "  - leaks-valgrind-*.txt: 'definitely lost' blocks with allocation paths"
echo "  - lm-test-output.txt:   Last test function that printed before hang"
echo "  - multilingual-asan.txt: SEGV stack trace"
echo ""
echo "To share results, zip the debug-output/ folder."
