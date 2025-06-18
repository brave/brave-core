# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import override_utils


@override_utils.override_method(Member)
def process(self,
            original_method,
            callbacks,
            functions_are_properties=False,
            use_returns_async=False):
    """Override adding support to `override` attribute.

  This override allows users to specify an `override` C++ type for binary blobs,
  rather than the usual use of a vector of bytes. This is used to store 64-bit
  integers to and from base::Value binary blobs.
  """
    [name,
     properties] = original_method(self, callbacks, functions_are_properties,
                                   use_returns_async)

    if self.node.GetProperty('override'):
        properties['override'] = self.node.GetProperty('override')

    return (name, properties)
