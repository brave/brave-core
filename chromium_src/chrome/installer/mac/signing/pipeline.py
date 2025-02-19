# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import os.path
import override_utils


@override_utils.override_function(globals())
def _customize_and_sign_chrome(original_function, paths, dist_config, *args):
    base_config = dist_config.base_config
    # This also serves as a safeguard that .enable_universal_app_dir exists:
    value_before = base_config.enable_universal_app_dir
    base_config.enable_universal_app_dir = True
    try:
        return original_function(paths, dist_config, *args)
    finally:
        base_config.enable_universal_app_dir = value_before


@override_utils.override_function(globals())
def _create_pkgbuild_scripts(original_function, paths, dist_config):
    orig_packaging_dir = paths.packaging_dir

    def new_packaging_dir(*args, **kwargs):
        orig = orig_packaging_dir(*args, **kwargs)
        return os.path.join(orig, 'brave')

    paths.packaging_dir = new_packaging_dir
    try:
        return original_function(paths, dist_config)
    finally:
        paths.packaging_dir = orig_packaging_dir
