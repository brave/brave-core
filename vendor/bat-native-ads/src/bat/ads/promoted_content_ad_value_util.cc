/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/promoted_content_ad_value_util.h"

#include "bat/ads/promoted_content_ad_info.h"
#include "url/gurl.h"

namespace ads {

namespace {

constexpr char kTypeKey[] = "type";
constexpr char kPlacementIdKey[] = "uuid";
constexpr char kCreativeInstanceIdKey[] = "creative_instance_id";
constexpr char kCreativeSetIdKey[] = "creative_set_id";
constexpr char kCampaignIdKey[] = "campaign_id";
constexpr char kAdvertiserIdKey[] = "advertiser_id";
constexpr char kSegmentKey[] = "segment";
constexpr char kTitleKey[] = "title";
constexpr char kDescriptionnKey[] = "description";
constexpr char kTargetUrlKey[] = "target_url";

}  // namespace

base::Value::Dict PromotedContentAdToValue(const PromotedContentAdInfo& ad) {
  base::Value::Dict dict;

  dict.Set(kTypeKey, ad.type.ToString());
  dict.Set(kPlacementIdKey, ad.placement_id);
  dict.Set(kCreativeInstanceIdKey, ad.creative_instance_id);
  dict.Set(kCreativeSetIdKey, ad.creative_set_id);
  dict.Set(kCampaignIdKey, ad.campaign_id);
  dict.Set(kAdvertiserIdKey, ad.advertiser_id);
  dict.Set(kSegmentKey, ad.segment);
  dict.Set(kTitleKey, ad.title);
  dict.Set(kDescriptionnKey, ad.description);
  dict.Set(kTargetUrlKey, ad.target_url.spec());

  return dict;
}

PromotedContentAdInfo PromotedContentAdFromValue(
    const base::Value::Dict& root) {
  PromotedContentAdInfo ad;

  if (const auto* value = root.FindString("type")) {
    ad.type = AdType(*value);
  }

  if (const auto* value = root.FindString("uuid")) {
    ad.placement_id = *value;
  }

  if (const auto* value = root.FindString("creative_instance_id")) {
    ad.creative_instance_id = *value;
  }

  if (const auto* value = root.FindString("creative_set_id")) {
    ad.creative_set_id = *value;
  }

  if (const auto* value = root.FindString("campaign_id")) {
    ad.campaign_id = *value;
  }

  if (const auto* value = root.FindString("advertiser_id")) {
    ad.advertiser_id = *value;
  }

  if (const auto* value = root.FindString("segment")) {
    ad.segment = *value;
  }

  if (const auto* value = root.FindString("title")) {
    ad.title = *value;
  }

  if (const auto* value = root.FindString("description")) {
    ad.description = *value;
  }

  if (const auto* value = root.FindString("target_url")) {
    ad.target_url = GURL(*value);
  }

  return ad;
}

}  // namespace ads
