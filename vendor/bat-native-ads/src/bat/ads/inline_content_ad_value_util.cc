/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/inline_content_ad_value_util.h"

#include "bat/ads/inline_content_ad_info.h"

namespace ads {

namespace {

constexpr char kTypeKey[] = "type";
constexpr char kPlacementIdKey[] = "uuid";
constexpr char kCreativeInstanceIdKey[] = "creativeInstanceId";
constexpr char kCreativeSetIdKey[] = "creativeSetId";
constexpr char kCampaignIdKey[] = "campaignId";
constexpr char kAdvertiserIdKey[] = "advertiserId";
constexpr char kSegmentKey[] = "segment";
constexpr char kTitleKey[] = "title";
constexpr char kDescriptionKey[] = "description";
constexpr char kImageUrlKey[] = "imageUrl";
constexpr char kDimensionsKey[] = "dimensions";
constexpr char kCtaTextKey[] = "ctaText";
constexpr char kTargetUrlKey[] = "targetUrl";

}  // namespace

base::Value::Dict InlineContentAdToValue(const InlineContentAdInfo& ad) {
  base::Value::Dict dict;

  dict.Set(kTypeKey, ad.type.ToString());
  dict.Set(kPlacementIdKey, ad.placement_id);
  dict.Set(kCreativeInstanceIdKey, ad.creative_instance_id);
  dict.Set(kCreativeSetIdKey, ad.creative_set_id);
  dict.Set(kCampaignIdKey, ad.campaign_id);
  dict.Set(kAdvertiserIdKey, ad.advertiser_id);
  dict.Set(kSegmentKey, ad.segment);
  dict.Set(kTitleKey, ad.title);
  dict.Set(kDescriptionKey, ad.description);
  dict.Set(kImageUrlKey, ad.image_url.spec());
  dict.Set(kDimensionsKey, ad.dimensions);
  dict.Set(kCtaTextKey, ad.cta_text);
  dict.Set(kTargetUrlKey, ad.target_url.spec());

  return dict;
}

InlineContentAdInfo InlineContentAdFromValue(const base::Value::Dict& root) {
  InlineContentAdInfo ad;

  if (const auto* value = root.FindString(kTypeKey)) {
    ad.type = AdType(*value);
  }

  if (const auto* value = root.FindString(kPlacementIdKey)) {
    ad.placement_id = *value;
  }

  if (const auto* value = root.FindString(kCreativeInstanceIdKey)) {
    ad.creative_instance_id = *value;
  }

  if (const auto* value = root.FindString(kCreativeSetIdKey)) {
    ad.creative_set_id = *value;
  }

  if (const auto* value = root.FindString(kCampaignIdKey)) {
    ad.campaign_id = *value;
  }

  if (const auto* value = root.FindString(kAdvertiserIdKey)) {
    ad.advertiser_id = *value;
  }

  if (const auto* value = root.FindString(kSegmentKey)) {
    ad.segment = *value;
  }

  if (const auto* value = root.FindString(kTitleKey)) {
    ad.title = *value;
  }

  if (const auto* value = root.FindString(kDescriptionKey)) {
    ad.description = *value;
  }

  if (const auto* value = root.FindString(kImageUrlKey)) {
    ad.image_url = GURL(*value);
  }

  if (const auto* value = root.FindString(kDimensionsKey)) {
    ad.dimensions = *value;
  }

  if (const auto* value = root.FindString(kCtaTextKey)) {
    ad.cta_text = *value;
  }

  if (const auto* value = root.FindString(kTargetUrlKey)) {
    ad.target_url = GURL(*value);
  }

  return ad;
}

}  // namespace ads
