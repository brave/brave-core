# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

import override_utils


@override_utils.override_method(MessageNode)
def _IsValidAttribute(self, original_method, name, value):
    if original_method(self, name, value):
        return True
    if name == 'webui':
        return True
    return False
