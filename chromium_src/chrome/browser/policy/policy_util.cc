/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/policy/policy_util.cc"

#include "base/feature_override.h"

namespace policy {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kDevicePolicyInvalidationWithDirectMessagesEnabled,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kDeviceLocalAccountPolicyInvalidationWithDirectMessagesEnabled,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kCbcmPolicyInvalidationWithDirectMessagesEnabled,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kUserPolicyInvalidationWithDirectMessagesEnabled,
     base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace policy
