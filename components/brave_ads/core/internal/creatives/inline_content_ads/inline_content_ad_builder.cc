/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"

namespace brave_ads {

InlineContentAdInfo BuildInlineContentAd(
    const CreativeInlineContentAdInfo& creative_ad) {
  const std::string placement_id =
      base::Uuid::GenerateRandomV4().AsLowercaseString();
  return BuildInlineContentAd(creative_ad, placement_id);
}

InlineContentAdInfo BuildInlineContentAd(
    const CreativeInlineContentAdInfo& creative_ad,
    const std::string& placement_id) {
  InlineContentAdInfo ad;

  ad.type = mojom::AdType::kInlineContentAd;
  ad.placement_id = placement_id;
  ad.creative_instance_id = creative_ad.creative_instance_id;
  ad.creative_set_id = creative_ad.creative_set_id;
  ad.campaign_id = creative_ad.campaign_id;
  ad.advertiser_id = creative_ad.advertiser_id;
  ad.segment = creative_ad.segment;
  ad.title = creative_ad.title;
  ad.description = creative_ad.description;
  ad.image_url = creative_ad.image_url;
  ad.dimensions = creative_ad.dimensions;
  ad.cta_text = creative_ad.cta_text;
  ad.target_url = creative_ad.target_url;

  return ad;
}

}  // namespace brave_ads
