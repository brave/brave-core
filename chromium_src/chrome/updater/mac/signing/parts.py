# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from signing.unbranded_config import UnbrandedCodeSignConfig
from signing_helper import BraveCodesignConfig

import override_utils


@override_utils.override_function(globals())
def get_parts(orig_fn, config):
    result = orig_fn(config)
    # Mirror the actual values of GoogleUpdater's binaries:
    for product in result:
        if product.identifier == config.keystone_app_name + 'Agent':
            product.identifier = 'com.brave.Keystone.Agent'
            product.identifier_requirement = True
        elif product.identifier == config.keystone_app_name:
            product.identifier = 'com.brave.Keystone'
        elif product.identifier == config.base_bundle_id:
            # This branch captures the top-level .app and the `launcher` binary.
            product.identifier_requirement = True
    return result
