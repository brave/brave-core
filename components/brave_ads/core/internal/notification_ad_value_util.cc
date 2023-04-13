/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/notification_ad_value_util.h"

#include "brave/components/brave_ads/core/notification_ad_constants.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {
constexpr char kTypeKey[] = "type";
}  // namespace

base::Value::Dict NotificationAdToValue(const NotificationAdInfo& ad) {
  base::Value::Dict dict;

  dict.Set(kTypeKey, ad.type.ToString());
  dict.Set(kNotificationAdPlacementIdKey, ad.placement_id);
  dict.Set(kNotificationAdCreativeInstanceIdKey, ad.creative_instance_id);
  dict.Set(kNotificationAdCreativeSetIdKey, ad.creative_set_id);
  dict.Set(kNotificationAdCampaignIdKey, ad.campaign_id);
  dict.Set(kNotificationAdAdvertiserIdKey, ad.advertiser_id);
  dict.Set(kNotificationAdSegmentKey, ad.segment);
  dict.Set(kNotificationAdTitleKey, ad.title);
  dict.Set(kNotificationAdBodyKey, ad.body);
  dict.Set(kNotificationAdTargetUrlKey, ad.target_url.spec());

  return dict;
}

base::Value::List NotificationAdsToValue(
    const base::circular_deque<NotificationAdInfo>& ads) {
  base::Value::List list;

  for (const auto& ad : ads) {
    list.Append(NotificationAdToValue(ad));
  }

  return list;
}

NotificationAdInfo NotificationAdFromValue(const base::Value::Dict& root) {
  NotificationAdInfo ad;

  if (const auto* value = root.FindString(kTypeKey)) {
    ad.type = AdType(*value);
  }

  if (const auto* value = root.FindString(kNotificationAdPlacementIdKey)) {
    ad.placement_id = *value;
  }

  if (const auto* value =
          root.FindString(kNotificationAdCreativeInstanceIdKey)) {
    ad.creative_instance_id = *value;
  }

  if (const auto* value = root.FindString(kNotificationAdCreativeSetIdKey)) {
    ad.creative_set_id = *value;
  }

  if (const auto* value = root.FindString(kNotificationAdCampaignIdKey)) {
    ad.campaign_id = *value;
  }

  if (const auto* value = root.FindString(kNotificationAdAdvertiserIdKey)) {
    ad.advertiser_id = *value;
  }

  if (const auto* value = root.FindString(kNotificationAdSegmentKey)) {
    ad.segment = *value;
  }

  if (const auto* value = root.FindString(kNotificationAdTitleKey)) {
    ad.title = *value;
  }

  if (const auto* value = root.FindString(kNotificationAdBodyKey)) {
    ad.body = *value;
  }

  if (const auto* value = root.FindString(kNotificationAdTargetUrlKey)) {
    ad.target_url = GURL(*value);
  }

  return ad;
}

base::circular_deque<NotificationAdInfo> NotificationAdsFromValue(
    const base::Value::List& list) {
  base::circular_deque<NotificationAdInfo> ads;

  for (const auto& item : list) {
    const auto* dict = item.GetIfDict();
    if (!dict) {
      continue;
    }

    ads.push_back(NotificationAdFromValue(*dict));
  }

  return ads;
}

}  // namespace brave_ads
