/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/components/permissions/features.cc"

#include "base/feature_override.h"

namespace permissions::features {

// Enables the option of an automatic permission expiration time.
BASE_FEATURE(kPermissionLifetime,
             "PermissionLifetime",
             base::FEATURE_ENABLED_BY_DEFAULT);

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kPermissionOnDeviceNotificationPredictions,
     base::FEATURE_DISABLED_BY_DEFAULT},
#if !BUILDFLAG(IS_ANDROID)
    {kPermissionsPromptSurvey, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
    {kShowRelatedWebsiteSetsPermissionGrants,
     base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace permissions::features
