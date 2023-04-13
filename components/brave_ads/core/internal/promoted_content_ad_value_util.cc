/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/promoted_content_ad_value_util.h"

#include "brave/components/brave_ads/core/promoted_content_ad_constants.h"
#include "brave/components/brave_ads/core/promoted_content_ad_info.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {
constexpr char kTypeKey[] = "type";
}  // namespace

base::Value::Dict PromotedContentAdToValue(const PromotedContentAdInfo& ad) {
  base::Value::Dict dict;

  dict.Set(kTypeKey, ad.type.ToString());
  dict.Set(kPromotedContentAdPlacementIdKey, ad.placement_id);
  dict.Set(kPromotedContentAdCreativeInstanceIdKey, ad.creative_instance_id);
  dict.Set(kPromotedContentAdCreativeSetIdKey, ad.creative_set_id);
  dict.Set(kPromotedContentAdCampaignIdKey, ad.campaign_id);
  dict.Set(kPromotedContentAdAdvertiserIdKey, ad.advertiser_id);
  dict.Set(kPromotedContentAdSegmentKey, ad.segment);
  dict.Set(kPromotedContentAdTitleKey, ad.title);
  dict.Set(kPromotedContentAdDescriptionnKey, ad.description);
  dict.Set(kPromotedContentAdTargetUrlKey, ad.target_url.spec());

  return dict;
}

PromotedContentAdInfo PromotedContentAdFromValue(
    const base::Value::Dict& root) {
  PromotedContentAdInfo ad;

  if (const auto* value = root.FindString(kTypeKey)) {
    ad.type = AdType(*value);
  }

  if (const auto* value = root.FindString(kPromotedContentAdPlacementIdKey)) {
    ad.placement_id = *value;
  }

  if (const auto* value =
          root.FindString(kPromotedContentAdCreativeInstanceIdKey)) {
    ad.creative_instance_id = *value;
  }

  if (const auto* value = root.FindString(kPromotedContentAdCreativeSetIdKey)) {
    ad.creative_set_id = *value;
  }

  if (const auto* value = root.FindString(kPromotedContentAdCampaignIdKey)) {
    ad.campaign_id = *value;
  }

  if (const auto* value = root.FindString(kPromotedContentAdAdvertiserIdKey)) {
    ad.advertiser_id = *value;
  }

  if (const auto* value = root.FindString(kPromotedContentAdSegmentKey)) {
    ad.segment = *value;
  }

  if (const auto* value = root.FindString(kPromotedContentAdTitleKey)) {
    ad.title = *value;
  }

  if (const auto* value = root.FindString(kPromotedContentAdDescriptionnKey)) {
    ad.description = *value;
  }

  if (const auto* value = root.FindString(kPromotedContentAdTargetUrlKey)) {
    ad.target_url = GURL(*value);
  }

  return ad;
}

}  // namespace brave_ads
