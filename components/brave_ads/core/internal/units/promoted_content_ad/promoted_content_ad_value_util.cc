/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/units/promoted_content_ad/promoted_content_ad_value_util.h"

#include "brave/components/brave_ads/core/public/units/promoted_content_ad/promoted_content_ad_constants.h"
#include "brave/components/brave_ads/core/public/units/promoted_content_ad/promoted_content_ad_info.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {
constexpr char kTypeKey[] = "type";
}  // namespace

base::Value::Dict PromotedContentAdToValue(const PromotedContentAdInfo& ad) {
  return base::Value::Dict()
      .Set(kTypeKey, ad.type.ToString())
      .Set(kPromotedContentAdPlacementIdKey, ad.placement_id)
      .Set(kPromotedContentAdCreativeInstanceIdKey, ad.creative_instance_id)
      .Set(kPromotedContentAdCreativeSetIdKey, ad.creative_set_id)
      .Set(kPromotedContentAdCampaignIdKey, ad.campaign_id)
      .Set(kPromotedContentAdAdvertiserIdKey, ad.advertiser_id)
      .Set(kPromotedContentAdSegmentKey, ad.segment)
      .Set(kPromotedContentAdTitleKey, ad.title)
      .Set(kPromotedContentAdDescriptionnKey, ad.description)
      .Set(kPromotedContentAdTargetUrlKey, ad.target_url.spec());
}

PromotedContentAdInfo PromotedContentAdFromValue(
    const base::Value::Dict& dict) {
  PromotedContentAdInfo ad;

  if (const auto* const value = dict.FindString(kTypeKey)) {
    ad.type = AdType(*value);
  }

  if (const auto* const value =
          dict.FindString(kPromotedContentAdPlacementIdKey)) {
    ad.placement_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kPromotedContentAdCreativeInstanceIdKey)) {
    ad.creative_instance_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kPromotedContentAdCreativeSetIdKey)) {
    ad.creative_set_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kPromotedContentAdCampaignIdKey)) {
    ad.campaign_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kPromotedContentAdAdvertiserIdKey)) {
    ad.advertiser_id = *value;
  }

  if (const auto* const value = dict.FindString(kPromotedContentAdSegmentKey)) {
    ad.segment = *value;
  }

  if (const auto* const value = dict.FindString(kPromotedContentAdTitleKey)) {
    ad.title = *value;
  }

  if (const auto* const value =
          dict.FindString(kPromotedContentAdDescriptionnKey)) {
    ad.description = *value;
  }

  if (const auto* const value =
          dict.FindString(kPromotedContentAdTargetUrlKey)) {
    ad.target_url = GURL(*value);
  }

  return ad;
}

}  // namespace brave_ads
