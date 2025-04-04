# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

from signing.unbranded_config import UnbrandedCodeSignConfig
from signing_helper import BraveCodesignConfig

import override_utils


@override_utils.override_function(globals())
def get_class(_):
    return BraveUpdaterCodeSignConfig


class BraveUpdaterCodeSignConfig(BraveCodesignConfig, UnbrandedCodeSignConfig):
    pass
