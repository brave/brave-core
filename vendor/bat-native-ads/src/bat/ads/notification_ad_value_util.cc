/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/notification_ad_value_util.h"

#include "bat/ads/notification_ad_info.h"
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
constexpr char kBodyKey[] = "body";
constexpr char kTargetUrlKey[] = "target_url";

}  // namespace

base::Value::Dict NotificationAdToValue(const NotificationAdInfo& ad) {
  base::Value::Dict dict;

  dict.Set(kTypeKey, ad.type.ToString());
  dict.Set(kPlacementIdKey, ad.placement_id);
  dict.Set(kCreativeInstanceIdKey, ad.creative_instance_id);
  dict.Set(kCreativeSetIdKey, ad.creative_set_id);
  dict.Set(kCampaignIdKey, ad.campaign_id);
  dict.Set(kAdvertiserIdKey, ad.advertiser_id);
  dict.Set(kSegmentKey, ad.segment);
  dict.Set(kTitleKey, ad.title);
  dict.Set(kBodyKey, ad.body);
  dict.Set(kTargetUrlKey, ad.target_url.spec());

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

  if (const auto* value = root.FindString(kBodyKey)) {
    ad.body = *value;
  }

  if (const auto* value = root.FindString(kTargetUrlKey)) {
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

}  // namespace ads
