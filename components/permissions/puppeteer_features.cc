/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/puppeteer_features.h"

#include "build/build_config.h"

namespace permissions::features {

BASE_FEATURE(kBravePuppeteerPermission,
             "BravePuppeteerPermission",
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
             base::FEATURE_ENABLED_BY_DEFAULT);
#else
             base::FEATURE_DISABLED_BY_DEFAULT);
#endif

bool IsBravePuppeteerPermissionEnabled() {
  return base::FeatureList::IsEnabled(features::kBravePuppeteerPermission);
}

}  // namespace permissions::features