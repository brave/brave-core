#!/usr/bin/env bash

set -euo pipefail

mkdir -p dist
tar -czvf $1 --ignore-failed-read *.so apks android_clang_*/*.so android_clang_*/lib.unstripped lib.unstripped android_chrome_versions.txt
