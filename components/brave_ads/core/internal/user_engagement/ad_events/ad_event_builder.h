/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_BUILDER_H_

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

struct AdEventInfo;
struct AdInfo;

// Builds a new `AdEventInfo` for the given `ad`, `mojom_confirmation_type`, and
// `created_at` time.
AdEventInfo BuildAdEvent(const AdInfo& ad,
                         mojom::ConfirmationType mojom_confirmation_type,
                         base::Time created_at);

// Rebuilds an existing `AdEventInfo` with a new `mojom_confirmation_type` and
// `created_at` time.
AdEventInfo RebuildAdEvent(const AdEventInfo& ad_event,
                           mojom::ConfirmationType mojom_confirmation_type,
                           base::Time created_at);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_BUILDER_H_
