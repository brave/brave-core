# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import override_utils


@override_utils.override_method(Type)
def __init__(self, original_method, parent, name, json, namespace,
             input_origin):
    """Adding `override` field to Type model.

  This override plumbs the provided `override` field into the model Type. This
  is then used to specify a C++ type for binary blobs that are used to store
  64-bit integers, rather than the default vector of bytes.
  """

    original_method(self, parent, name, json, namespace, input_origin)

    json_type = json.get('type', None)
    if json_type == 'binary':
        self.override = json.get('override', None)
