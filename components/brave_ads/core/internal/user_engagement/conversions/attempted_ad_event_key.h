/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_ATTEMPTED_AD_EVENT_KEY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_ATTEMPTED_AD_EVENT_KEY_H_

#include <string>
#include <tuple>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

// Identifies an ad event by placement ID, confirmation type, and creation
// time, so a repeat conversion check cannot convert the same one twice.
using AttemptedAdEventKey = std::tuple<std::string /*placement_id*/,
                                       mojom::ConfirmationType,
                                       base::Time /*created_at*/>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_ATTEMPTED_AD_EVENT_KEY_H_
