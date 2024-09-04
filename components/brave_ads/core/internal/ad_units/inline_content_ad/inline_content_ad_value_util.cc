/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_value_util.h"

#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_constants.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"

namespace brave_ads {

base::Value::Dict InlineContentAdToValue(const InlineContentAdInfo& ad) {
  return base::Value::Dict()
      .Set(kInlineContentAdTypeKey, ToString(ad.type))
      .Set(kInlineContentAdPlacementIdKey, ad.placement_id)
      .Set(kInlineContentAdCreativeInstanceIdKey, ad.creative_instance_id)
      .Set(kInlineContentAdCreativeSetIdKey, ad.creative_set_id)
      .Set(kInlineContentAdCampaignIdKey, ad.campaign_id)
      .Set(kInlineContentAdAdvertiserIdKey, ad.advertiser_id)
      .Set(kInlineContentAdSegmentKey, ad.segment)
      .Set(kInlineContentAdTitleKey, ad.title)
      .Set(kInlineContentAdDescriptionKey, ad.description)
      .Set(kInlineContentAdImageUrlKey, ad.image_url.spec())
      .Set(kInlineContentAdDimensionsKey, ad.dimensions)
      .Set(kInlineContentAdCtaTextKey, ad.cta_text)
      .Set(kInlineContentAdTargetUrlKey, ad.target_url.spec());
}

InlineContentAdInfo InlineContentAdFromValue(const base::Value::Dict& dict) {
  InlineContentAdInfo ad;

  if (const auto* const value = dict.FindString(kInlineContentAdTypeKey)) {
    ad.type = ToMojomAdType(*value);
  }

  if (const auto* const value =
          dict.FindString(kInlineContentAdPlacementIdKey)) {
    ad.placement_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kInlineContentAdCreativeInstanceIdKey)) {
    ad.creative_instance_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kInlineContentAdCreativeSetIdKey)) {
    ad.creative_set_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kInlineContentAdCampaignIdKey)) {
    ad.campaign_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kInlineContentAdAdvertiserIdKey)) {
    ad.advertiser_id = *value;
  }

  if (const auto* const value = dict.FindString(kInlineContentAdSegmentKey)) {
    ad.segment = *value;
  }

  if (const auto* const value = dict.FindString(kInlineContentAdTitleKey)) {
    ad.title = *value;
  }

  if (const auto* const value =
          dict.FindString(kInlineContentAdDescriptionKey)) {
    ad.description = *value;
  }

  if (const auto* const value = dict.FindString(kInlineContentAdImageUrlKey)) {
    ad.image_url = GURL(*value);
  }

  if (const auto* const value =
          dict.FindString(kInlineContentAdDimensionsKey)) {
    ad.dimensions = *value;
  }

  if (const auto* const value = dict.FindString(kInlineContentAdCtaTextKey)) {
    ad.cta_text = *value;
  }

  if (const auto* const value = dict.FindString(kInlineContentAdTargetUrlKey)) {
    ad.target_url = GURL(*value);
  }

  return ad;
}

}  // namespace brave_ads
