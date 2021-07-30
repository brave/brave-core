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
    const CreativeInlineContentAdInfo& creative_inline_content_ad) {
  const std::string uuid = base::GenerateGUID();
  return BuildInlineContentAd(creative_inline_content_ad, uuid);
}

InlineContentAdInfo BuildInlineContentAd(
    const CreativeInlineContentAdInfo& creative_inline_content_ad,
    const std::string& uuid) {
  InlineContentAdInfo inline_content_ad;

  inline_content_ad.type = AdType::kInlineContentAd;
  inline_content_ad.uuid = uuid;
  inline_content_ad.creative_instance_id =
      creative_inline_content_ad.creative_instance_id;
  inline_content_ad.creative_set_id =
      creative_inline_content_ad.creative_set_id;
  inline_content_ad.campaign_id = creative_inline_content_ad.campaign_id;
  inline_content_ad.advertiser_id = creative_inline_content_ad.advertiser_id;
  inline_content_ad.segment = creative_inline_content_ad.segment;
  inline_content_ad.title = creative_inline_content_ad.title;
  inline_content_ad.description = creative_inline_content_ad.description;
  inline_content_ad.image_url = creative_inline_content_ad.image_url;
  inline_content_ad.dimensions = creative_inline_content_ad.dimensions;
  inline_content_ad.cta_text = creative_inline_content_ad.cta_text;
  inline_content_ad.target_url = creative_inline_content_ad.target_url;

  return inline_content_ad;
}

}  // namespace ads
