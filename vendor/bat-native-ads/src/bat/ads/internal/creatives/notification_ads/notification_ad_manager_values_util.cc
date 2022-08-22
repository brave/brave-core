/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/notification_ads/notification_ad_manager_values_util.h"

#include <utility>

#include "bat/ads/notification_ad_info.h"
#include "url/gurl.h"

namespace ads {

namespace {

constexpr char kPlacementIdKey[] = "placement_id";
constexpr char kCreativeInstanceIdKey[] = "creative_instance_id";
constexpr char kCreativeSetIdKey[] = "creative_set_id";
constexpr char kCampaignIdKey[] = "campaign_id";
constexpr char kAdvertiserIdKey[] = "advertiser_id";
constexpr char kSegmentKey[] = "segment";
constexpr char kTitleKey[] = "title";
constexpr char kBodyKey[] = "body";
constexpr char kTargetUrlKey[] = "target_url";

}  // namespace

base::circular_deque<NotificationAdInfo> NotificationAdsFromValue(
    const base::Value::List& list) {
  base::circular_deque<NotificationAdInfo> ads;

  for (const auto& item : list) {
    const auto* dict = item.GetIfDict();
    if (!dict) {
      continue;
    }

    NotificationAdInfo ad;

    if (const auto* value = dict->FindString(kPlacementIdKey)) {
      ad.placement_id = *value;
    }

    if (const auto* value = dict->FindString(kCreativeInstanceIdKey)) {
      ad.creative_instance_id = *value;
    }

    if (const auto* value = dict->FindString(kCreativeSetIdKey)) {
      ad.creative_set_id = *value;
    }

    if (const auto* value = dict->FindString(kCampaignIdKey)) {
      ad.campaign_id = *value;
    }

    if (const auto* value = dict->FindString(kAdvertiserIdKey)) {
      ad.advertiser_id = *value;
    }

    if (const auto* value = dict->FindString(kSegmentKey)) {
      ad.segment = *value;
    }

    if (const auto* value = dict->FindString(kTitleKey)) {
      ad.title = *value;
    }

    if (const auto* value = dict->FindString(kBodyKey)) {
      ad.body = *value;
    }

    if (const auto* value = dict->FindString(kTargetUrlKey)) {
      ad.target_url = GURL(*value);
    }

    ads.push_back(ad);
  }

  return ads;
}

base::Value::List NotificationAdsToValue(
    const base::circular_deque<NotificationAdInfo>& ads) {
  base::Value::List list;

  for (const auto& ad : ads) {
    base::Value::Dict dict;

    dict.Set(kPlacementIdKey, ad.placement_id);
    dict.Set(kCreativeInstanceIdKey, ad.creative_instance_id);
    dict.Set(kCreativeSetIdKey, ad.creative_set_id);
    dict.Set(kCampaignIdKey, ad.campaign_id);
    dict.Set(kAdvertiserIdKey, ad.advertiser_id);
    dict.Set(kSegmentKey, ad.segment);
    dict.Set(kTitleKey, ad.title);
    dict.Set(kBodyKey, ad.body);
    dict.Set(kTargetUrlKey, ad.target_url.spec());

    list.Append(std::move(dict));
  }

  return list;
}

}  // namespace ads
