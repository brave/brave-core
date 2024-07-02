/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/components/content_settings/core/common/features.cc"

#include "base/feature_override.h"

namespace content_settings {
namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kTrackingProtection3pcd, base::FEATURE_DISABLED_BY_DEFAULT},
    {kUserBypassUI, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace features

// Brave implements a strictier policy to not leak blocked permissions into
// incognito profiles. This feature (when enabled) restores the original
// Chromium implementation which makes INHERIT_IF_LESS_PERMISSIVE inherit
// blocked permissions in incognito profile.
BASE_FEATURE(kAllowIncognitoPermissionInheritance,
             "AllowIncognitoPermissionInheritance",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace content_settings
