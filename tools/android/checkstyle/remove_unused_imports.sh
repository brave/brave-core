#!/usr/bin/env bash

# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

SCRIPT_DIR="$( dirname -- "${BASH_SOURCE[0]}"; )"
SCRIPT_DIR="$( realpath -e -- "$SCRIPT_DIR"; )"

CHROMIUM_CHECKSTYLE_DIR="$SCRIPT_DIR/../../../../tools/android/checkstyle"
CHROMIUM_CHECKSTYLE_DIR="$( realpath -e -- "$CHROMIUM_CHECKSTYLE_DIR"; )"
export PYTHONPATH="$CHROMIUM_CHECKSTYLE_DIR"

python3  "$SCRIPT_DIR/remove_unused_imports.py"
