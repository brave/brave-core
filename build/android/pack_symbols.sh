#!/usr/bin/env bash

# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

set -euo pipefail

mkdir -p "${1:?}"

shopt -s nullglob

tar -czvf "${2:?}" ./*.so apks android_clang_*/*.so android_clang_*/lib.unstripped lib.unstripped android_chrome_versions.txt
