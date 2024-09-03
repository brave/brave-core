/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_BUILDER_H_

#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

struct AdEventInfo;
struct AdInfo;

// Builds a new `AdEventInfo` for the given `ad`, `confirmation_type`, and
// `created_at` time.
AdEventInfo BuildAdEvent(const AdInfo& ad,
                         ConfirmationType confirmation_type,
                         base::Time created_at);

// Rebuilds an existing `AdEventInfo` with a new `confirmation_type` and
// `created_at` time.
AdEventInfo RebuildAdEvent(const AdEventInfo& ad_event,
                           ConfirmationType confirmation_type,
                           base::Time created_at);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_BUILDER_H_
