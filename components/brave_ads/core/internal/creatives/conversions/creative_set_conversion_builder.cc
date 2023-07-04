/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_builder.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"

namespace brave_ads {

absl::optional<CreativeSetConversionInfo> BuildCreativeSetConversion(
    const mojom::SearchResultAdInfoPtr& ad_mojom) {
  if (!ad_mojom || !ad_mojom->conversion) {
    return absl::nullopt;
  }

  CreativeSetConversionInfo creative_set_conversion;

  creative_set_conversion.id = ad_mojom->creative_set_id;

  creative_set_conversion.url_pattern = ad_mojom->conversion->url_pattern;

  creative_set_conversion.extract_verifiable_id =
      ad_mojom->conversion->extract_verifiable_id;

  creative_set_conversion.verifiable_advertiser_public_key_base64 =
      ad_mojom->conversion->verifiable_advertiser_public_key_base64;

  creative_set_conversion.observation_window =
      ad_mojom->conversion->observation_window;

  // Creative set conversions are built and saved when a search result ad is
  // viewed, i.e., now, so the conversion should expire after the observation
  // window has elapsed.
  creative_set_conversion.expire_at =
      base::Time::Now() + creative_set_conversion.observation_window;

  return creative_set_conversion;
}

}  // namespace brave_ads
