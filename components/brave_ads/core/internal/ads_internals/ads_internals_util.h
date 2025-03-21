/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INTERNALS_ADS_INTERNALS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INTERNALS_ADS_INTERNALS_UTIL_H_

#include "brave/components/brave_ads/core/public/service/ads_service_callback.h"

namespace brave_ads {

void BuildAdsInternals(GetInternalsCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INTERNALS_ADS_INTERNALS_UTIL_H_
