#!/usr/bin/env bash
set -euo pipefail
if [[ $# -lt 2 ]]; then
  echo "用法: build_gcc.sh input.zh output" >&2; exit 1
fi
python zhcc.py "$1" -o "$2"
echo "完成：$2"
