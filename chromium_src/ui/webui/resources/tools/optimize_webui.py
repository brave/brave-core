# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# This variable comes from the upstream file, so disable the pylint warning
# about undefined variables.
# pylint: disable=E0602
_BASE_EXCLUDES.extend([
    "chrome://resources/brave/leo/bundle.js", "//resources/brave/leo/bundle.js"
])
