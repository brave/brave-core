/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_builder.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

std::optional<CreativeSetConversionInfo>
FromMojomMaybeBuildCreativeSetConversion(
    const mojom::CreativeSearchResultAdInfoPtr& mojom_creative_ad) {
  if (!mojom_creative_ad || !mojom_creative_ad->creative_set_conversion) {
    return std::nullopt;
  }

  CreativeSetConversionInfo creative_set_conversion;

  creative_set_conversion.id = mojom_creative_ad->creative_set_id;

  creative_set_conversion.url_pattern =
      mojom_creative_ad->creative_set_conversion->url_pattern;

  creative_set_conversion.verifiable_advertiser_public_key_base64 =
      mojom_creative_ad->creative_set_conversion
          ->verifiable_advertiser_public_key_base64;

  creative_set_conversion.observation_window =
      mojom_creative_ad->creative_set_conversion->observation_window;

  // Creative set conversions are built and saved when a search result ad is
  // viewed by Rewards users or clicked by non-Rewards users, so the observation
  // window can start immediately.
  creative_set_conversion.expire_at =
      base::Time::Now() + creative_set_conversion.observation_window;

  return creative_set_conversion;
}

}  // namespace brave_ads
