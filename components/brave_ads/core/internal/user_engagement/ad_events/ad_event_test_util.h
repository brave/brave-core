/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_TEST_UTIL_H_

#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct AdInfo;

namespace test {

void RecordAdEvent(const AdInfo& ad,
                   mojom::ConfirmationType mojom_confirmation_type);

void RecordAdEvents(
    const AdInfo& ad,
    const std::vector<mojom::ConfirmationType>& mojom_confirmation_types);

void RecordAdEvents(const AdInfo& ad,
                    mojom::ConfirmationType mojom_confirmation_type,
                    int count);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_AD_EVENT_TEST_UTIL_H_
