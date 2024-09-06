/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_value_util.h"

#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_constants.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "url/gurl.h"

namespace brave_ads {

base::Value::Dict NotificationAdToValue(const NotificationAdInfo& ad) {
  return base::Value::Dict()
      .Set(kNotificationAdTypeKey, ToString(ad.type))
      .Set(kNotificationAdPlacementIdKey, ad.placement_id)
      .Set(kNotificationAdCreativeInstanceIdKey, ad.creative_instance_id)
      .Set(kNotificationAdCreativeSetIdKey, ad.creative_set_id)
      .Set(kNotificationAdCampaignIdKey, ad.campaign_id)
      .Set(kNotificationAdAdvertiserIdKey, ad.advertiser_id)
      .Set(kNotificationAdSegmentKey, ad.segment)
      .Set(kNotificationAdTitleKey, ad.title)
      .Set(kNotificationAdBodyKey, ad.body)
      .Set(kNotificationAdTargetUrlKey, ad.target_url.spec());
}

base::Value::List NotificationAdsToValue(
    const base::circular_deque<NotificationAdInfo>& ads) {
  base::Value::List list;
  list.reserve(ads.size());

  for (const auto& ad : ads) {
    list.Append(NotificationAdToValue(ad));
  }

  return list;
}

NotificationAdInfo NotificationAdFromValue(const base::Value::Dict& dict) {
  NotificationAdInfo ad;

  if (const auto* const value = dict.FindString(kNotificationAdTypeKey)) {
    ad.type = ToMojomAdType(*value);
  }

  if (const auto* const value =
          dict.FindString(kNotificationAdPlacementIdKey)) {
    ad.placement_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kNotificationAdCreativeInstanceIdKey)) {
    ad.creative_instance_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kNotificationAdCreativeSetIdKey)) {
    ad.creative_set_id = *value;
  }

  if (const auto* const value = dict.FindString(kNotificationAdCampaignIdKey)) {
    ad.campaign_id = *value;
  }

  if (const auto* const value =
          dict.FindString(kNotificationAdAdvertiserIdKey)) {
    ad.advertiser_id = *value;
  }

  if (const auto* const value = dict.FindString(kNotificationAdSegmentKey)) {
    ad.segment = *value;
  }

  if (const auto* const value = dict.FindString(kNotificationAdTitleKey)) {
    ad.title = *value;
  }

  if (const auto* const value = dict.FindString(kNotificationAdBodyKey)) {
    ad.body = *value;
  }

  if (const auto* const value = dict.FindString(kNotificationAdTargetUrlKey)) {
    ad.target_url = GURL(*value);
  }

  return ad;
}

base::circular_deque<NotificationAdInfo> NotificationAdsFromValue(
    const base::Value::List& list) {
  base::circular_deque<NotificationAdInfo> ads;

  for (const auto& value : list) {
    if (const auto* const dict = value.GetIfDict()) {
      ads.push_back(NotificationAdFromValue(*dict));
    }
  }

  return ads;
}

}  // namespace brave_ads
