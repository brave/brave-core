/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad_builder.h"

#include "base/guid.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_info.h"

namespace ads {

InlineContentAdInfo BuildInlineContentAd(
    const CreativeInlineContentAdInfo& creative_ad) {
  const std::string uuid = base::GenerateGUID();
  return BuildInlineContentAd(creative_ad, uuid);
}

InlineContentAdInfo BuildInlineContentAd(
    const CreativeInlineContentAdInfo& creative_ad,
    const std::string& uuid) {
  InlineContentAdInfo ad;

  ad.type = AdType::kInlineContentAd;
  ad.uuid = uuid;
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

}  // namespace ads
