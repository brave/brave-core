/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_AD_EVENTS_AD_EVENTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_AD_EVENTS_AD_EVENTS_H_

#include <vector>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

using AdEventCallback = base::OnceCallback<void(bool success)>;

class AdType;
class ConfirmationType;
struct AdEventInfo;
struct AdInfo;

void LogAdEvent(const AdInfo& ad,
                const ConfirmationType& confirmation_type,
                AdEventCallback callback);

void LogAdEvent(const AdEventInfo& ad_event, AdEventCallback callback);

void PurgeExpiredAdEvents(AdEventCallback callback);
void PurgeOrphanedAdEvents(mojom::AdType ad_type, AdEventCallback callback);

void RebuildAdEventHistoryFromDatabase();

void RecordAdEvent(const AdEventInfo& ad_event);

std::vector<base::Time> GetAdEventHistory(
    const AdType& ad_type,
    const ConfirmationType& confirmation_type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_AD_EVENTS_AD_EVENTS_H_
