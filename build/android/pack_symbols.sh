#!/usr/bin/env bash

# Copyright (c) 2024 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

set -euo pipefail

mkdir -p $1

objects_to_compress="*.so apks lib.unstripped android_chrome_versions.txt"

if ls android_clang_*/*.so 1> /dev/null 2>&1; then
    objects_to_compress="${objects_to_compress} android_clang_*/*.so"
fi

if ls android_clang_*/lib.unstripped 1> /dev/null 2>&1; then
    objects_to_compress="${objects_to_compress} android_clang_*/lib.unstripped"
fi

tar -czvf $2 --ignore-failed-read ${objects_to_compress}
