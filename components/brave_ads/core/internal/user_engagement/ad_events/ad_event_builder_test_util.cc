/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder_test_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::test {

AdEventInfo BuildAdEvent(const CreativeAdInfo& creative_ad,
                         mojom::AdType mojom_ad_type,
                         mojom::ConfirmationType mojom_confirmation_type,
                         base::Time created_at,
                         bool should_generate_random_uuids) {
  AdEventInfo ad_event;

  ad_event.type = mojom_ad_type;
  ad_event.confirmation_type = mojom_confirmation_type;
  ad_event.placement_id =
      RandomUuidOr(should_generate_random_uuids, kPlacementId);
  ad_event.campaign_id = creative_ad.campaign_id;
  ad_event.creative_set_id = creative_ad.creative_set_id;
  ad_event.creative_instance_id = creative_ad.creative_instance_id;
  ad_event.advertiser_id = creative_ad.advertiser_id;
  ad_event.segment = creative_ad.segment;
  ad_event.created_at = created_at;

  return ad_event;
}

}  // namespace brave_ads::test
