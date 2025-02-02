/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_value_util.h"

#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_constants.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

namespace brave_ads {

base::Value::Dict NewTabPageAdToValue(const NewTabPageAdInfo& ad) {
  return base::Value::Dict()
      .Set(kNewTabPageAdTypeKey, ToString(ad.type))
      .Set(kNewTabPageAdPlacementIdKey, ad.placement_id)
      .Set(kNewTabPageAdCreativeInstanceIdKey, ad.creative_instance_id)
      .Set(kNewTabPageAdCreativeSetIdKey, ad.creative_set_id)
      .Set(kNewTabPageAdCampaignIdKey, ad.campaign_id)
      .Set(kNewTabPageAdAdvertiserIdKey, ad.advertiser_id)
      .Set(kNewTabPageAdSegmentKey, ad.segment)
      .Set(kNewTabPageAdCompanyNameKey, ad.company_name)
      .Set(kNewTabPageAdAltKey, ad.alt)
      .Set(kNewTabPageAdTargetUrlKey, ad.target_url.spec());
}

NewTabPageAdInfo NewTabPageAdFromValue(const base::Value::Dict& dict) {
  NewTabPageAdInfo ad;

  if (const auto* const value = dict.FindString(kNewTabPageAdTypeKey)) {
    ad.type = ToMojomAdType(*value);
  }

  if (const auto* const value = dict.FindString(kNewTabPageAdPlacementIdKey)) {
    ad.placement_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kNewTabPageAdCreativeInstanceIdKey)) {
    ad.creative_instance_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kNewTabPageAdCreativeSetIdKey)) {
    ad.creative_set_id = *value;
  }

  if (const auto* const value = dict.FindString(kNewTabPageAdCampaignIdKey)) {
    ad.campaign_id = *value;
  }

  if (const auto* const value = dict.FindString(kNewTabPageAdAdvertiserIdKey)) {
    ad.advertiser_id = *value;
  }

  if (const auto* const value = dict.FindString(kNewTabPageAdSegmentKey)) {
    ad.segment = *value;
  }

  if (const auto* const value = dict.FindString(kNewTabPageAdCompanyNameKey)) {
    ad.company_name = *value;
  }

  if (const auto* const value = dict.FindString(kNewTabPageAdAltKey)) {
    ad.alt = *value;
  }

  if (const auto* const value = dict.FindString(kNewTabPageAdTargetUrlKey)) {
    ad.target_url = GURL(*value);
  }

  return ad;
}

}  // namespace brave_ads
