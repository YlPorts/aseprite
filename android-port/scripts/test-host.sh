#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cmake -S "$ROOT" -B "$ROOT/.build/host" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "$ROOT/.build/host" -j 2
ctest --test-dir "$ROOT/.build/host" --output-on-failure
