/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_INTERNALS_ADS_INTERNALS_VERBOSE_MODE_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_INTERNALS_ADS_INTERNALS_VERBOSE_MODE_FEATURE_H_

#include "base/feature_list.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

namespace brave_ads {

// Enables additional low-level debugging UI on brave://ads-internals.
// Off by default since it exposes ad-serving internals that are only useful for
// debugging.
BASE_DECLARE_FEATURE(kAdsInternalsVerboseModeFeature);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_INTERNALS_ADS_INTERNALS_VERBOSE_MODE_FEATURE_H_
