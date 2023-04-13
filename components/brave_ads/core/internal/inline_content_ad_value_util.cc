/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/inline_content_ad_value_util.h"

#include "brave/components/brave_ads/core/inline_content_ad_constants.h"
#include "brave/components/brave_ads/core/inline_content_ad_info.h"

namespace brave_ads {

namespace {
constexpr char kTypeKey[] = "type";
}  // namespace

base::Value::Dict InlineContentAdToValue(const InlineContentAdInfo& ad) {
  base::Value::Dict dict;

  dict.Set(kTypeKey, ad.type.ToString());
  dict.Set(kInlineContentAdPlacementIdKey, ad.placement_id);
  dict.Set(kInlineContentAdCreativeInstanceIdKey, ad.creative_instance_id);
  dict.Set(kInlineContentAdCreativeSetIdKey, ad.creative_set_id);
  dict.Set(kInlineContentAdCampaignIdKey, ad.campaign_id);
  dict.Set(kInlineContentAdAdvertiserIdKey, ad.advertiser_id);
  dict.Set(kInlineContentAdSegmentKey, ad.segment);
  dict.Set(kInlineContentAdTitleKey, ad.title);
  dict.Set(kInlineContentAdDescriptionKey, ad.description);
  dict.Set(kInlineContentAdImageUrlKey, ad.image_url.spec());
  dict.Set(kInlineContentAdDimensionsKey, ad.dimensions);
  dict.Set(kInlineContentAdCtaTextKey, ad.cta_text);
  dict.Set(kInlineContentAdTargetUrlKey, ad.target_url.spec());

  return dict;
}

InlineContentAdInfo InlineContentAdFromValue(const base::Value::Dict& root) {
  InlineContentAdInfo ad;

  if (const auto* value = root.FindString(kTypeKey)) {
    ad.type = AdType(*value);
  }

  if (const auto* value = root.FindString(kInlineContentAdPlacementIdKey)) {
    ad.placement_id = *value;
  }

  if (const auto* value =
          root.FindString(kInlineContentAdCreativeInstanceIdKey)) {
    ad.creative_instance_id = *value;
  }

  if (const auto* value = root.FindString(kInlineContentAdCreativeSetIdKey)) {
    ad.creative_set_id = *value;
  }

  if (const auto* value = root.FindString(kInlineContentAdCampaignIdKey)) {
    ad.campaign_id = *value;
  }

  if (const auto* value = root.FindString(kInlineContentAdAdvertiserIdKey)) {
    ad.advertiser_id = *value;
  }

  if (const auto* value = root.FindString(kInlineContentAdSegmentKey)) {
    ad.segment = *value;
  }

  if (const auto* value = root.FindString(kInlineContentAdTitleKey)) {
    ad.title = *value;
  }
  if (const auto* value = root.FindString(kInlineContentAdDescriptionKey)) {
    ad.description = *value;
  }

  if (const auto* value = root.FindString(kInlineContentAdImageUrlKey)) {
    ad.image_url = GURL(*value);
  }

  if (const auto* value = root.FindString(kInlineContentAdDimensionsKey)) {
    ad.dimensions = *value;
  }

  if (const auto* value = root.FindString(kInlineContentAdCtaTextKey)) {
    ad.cta_text = *value;
  }

  if (const auto* value = root.FindString(kInlineContentAdTargetUrlKey)) {
    ad.target_url = GURL(*value);
  }

  return ad;
}

}  // namespace brave_ads
