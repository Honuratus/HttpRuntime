#!/usr/bin/env bash
set -euo pipefail

PORT="${PORT:-8080}"
HOST="127.0.0.1"
BASE_URL="http://${HOST}:${PORT}"
SERVER_LOG="/tmp/fuckman-http.log"

echo "[1/7] Configure"
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "[2/7] Build"
cmake --build build

echo "[3/7] Prepare test files"
printf '\x01\x02\x7F\x8A\xFF' > build/test.bin

echo "[4/7] Start HTTP test server"
rm -f "$SERVER_LOG"

python3 -m http.server "$PORT" --bind "$HOST" --directory build >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!

cleanup() {
    if kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

echo "Waiting for server at ${BASE_URL}/test.bin"

SERVER_READY=0
for _ in {1..50}; do
    if curl -fsS "${BASE_URL}/test.bin" >/dev/null 2>&1; then
        SERVER_READY=1
        break
    fi

    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "HTTP server exited early."
        cat "$SERVER_LOG" || true
        exit 1
    fi

    sleep 0.2
done

if [ "$SERVER_READY" -ne 1 ]; then
    echo "HTTP test server did not become ready."
    cat "$SERVER_LOG" || true
    exit 1
fi

echo "[5/7] Unit/integration tests"
ctest --test-dir build --output-on-failure

echo "[6/7] Smoke tests"

./build/fuckman GET "https://example.com" \
    --expect-status 200 \
    --expect-body "Example Domain"

set +e
./build/fuckman GET "https://example.com" \
    --expect-status 404 \
    --expect-body "Example Domain"
FAIL_CODE=$?
set -e

if [ "$FAIL_CODE" -eq 0 ]; then
    echo "Expected failing assertion test to return non-zero, got 0."
    exit 1
fi

./build/fuckman GET "${BASE_URL}/test.bin" \
    --expect-status 200 \
    --expect-body hex:7F8A

echo "[7/7] Valgrind"

if command -v valgrind >/dev/null 2>&1; then
    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --error-exitcode=99 \
        --suppressions=tools/valgrind.supp \
        ./build/fuckman GET "${BASE_URL}/test.bin" \
            --expect-status 200 \
            --expect-body hex:7F8A
else
    echo "valgrind not found, skipping."
fi

echo
echo "All checks passed."