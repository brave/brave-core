/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"

namespace brave_ads {

ConversionInfo BuildConversion(
    const AdEventInfo& ad_event,
    std::optional<VerifiableConversionInfo> verifiable_conversion) {
  ConversionInfo conversion;

  conversion.ad_type = ad_event.type;
  conversion.creative_instance_id = ad_event.creative_instance_id;
  conversion.creative_set_id = ad_event.creative_set_id;
  conversion.campaign_id = ad_event.campaign_id;
  conversion.advertiser_id = ad_event.advertiser_id;
  conversion.segment = ad_event.segment;
  conversion.action_type = ToConversionActionType(ad_event.confirmation_type);
  conversion.verifiable = std::move(verifiable_conversion);

  return conversion;
}

}  // namespace brave_ads
