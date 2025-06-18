# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/. */

import override_utils


@override_utils.override_method(CppTypeGenerator)
def GetCppType(self, original_method, type_, is_optional=False):
    """Override to handle binary types with overrides.

    This override allows us to use a specific override type for binary fields
    that are being used as storage for 64-bit integers. The type itself is
    specified as part of the field attribute (usually [override=int64_t] or
    similar).
    """
    if type_.property_type == PropertyType.BINARY and type_.override:
        return type_.override

    return original_method(self, type_, is_optional)
