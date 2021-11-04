/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "../../../../../../chrome/browser/apps/app_discovery_service/app_discovery_features.cc"

#include "base/feature_override.h"

namespace apps {

#if !defined(OS_ANDROID)
DISABLE_FEATURE_BY_DEFAULT(kAppDiscoveryRemoteUrlSearch);
#endif

}  // namespace apps
