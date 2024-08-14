/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENTS_H_

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads {

using AdEventCallback = base::OnceCallback<void(bool success)>;

struct AdEventInfo;
struct AdInfo;

void RecordAdEvent(const AdInfo& ad,
                   ConfirmationType confirmation_type,
                   AdEventCallback callback);
void RecordAdEvent(const AdEventInfo& ad_event, AdEventCallback callback);

void PurgeOrphanedAdEvents(mojom::AdType ad_type, AdEventCallback callback);
void PurgeOrphanedAdEvents(const std::vector<std::string>& placement_ids,
                           AdEventCallback callback);
void PurgeAllOrphanedAdEvents(AdEventCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENTS_H_
