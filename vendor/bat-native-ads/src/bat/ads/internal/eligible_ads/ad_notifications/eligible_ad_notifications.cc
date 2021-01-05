/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications.h"

#include <stdint.h>

#include <map>
#include <string>

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/eligible_ads/eligible_ads_util.h"
#include "bat/ads/internal/frequency_capping/ad_notifications/ad_notifications_frequency_capping.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_notifications {

namespace {

bool ShouldCapLastDeliveredAd(
    const CreativeAdNotificationList& ads) {
  return ads.size() != 1;
}

}  // namespace

EligibleAds::EligibleAds(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting)
    : subdivision_targeting_(subdivision_targeting) {
  DCHECK(subdivision_targeting_);
}

EligibleAds::~EligibleAds() = default;

CreativeAdNotificationList EligibleAds::Get(
    const CreativeAdNotificationList& ads,
    const CreativeAdInfo& last_delivered_ad,
    const AdEventList& ad_events) {
  CreativeAdNotificationList eligible_ads = ads;
  if (eligible_ads.empty()) {
    return eligible_ads;
  }

  eligible_ads = RemoveSeenAdvertisersAndRoundRobinIfNeeded(eligible_ads);

  eligible_ads = RemoveSeenAdsAndRoundRobinIfNeeded(eligible_ads);

  eligible_ads = FrequencyCap(eligible_ads, ShouldCapLastDeliveredAd(ads) ?
      last_delivered_ad : CreativeAdInfo(), ad_events);

  return eligible_ads;
}

///////////////////////////////////////////////////////////////////////////////

CreativeAdNotificationList
EligibleAds::RemoveSeenAdvertisersAndRoundRobinIfNeeded(
    const CreativeAdNotificationList& ads) const {
  const std::map<std::string, uint64_t> seen_advertisers =
      Client::Get()->GetSeenAdvertisers();

  CreativeAdNotificationList eligible_ads =
      FilterSeenAdvertisers(ads, seen_advertisers);

  if (eligible_ads.empty()) {
    BLOG(1, "All advertisers have been shown, so round robin");
    Client::Get()->ResetSeenAdvertisers(ads);
    eligible_ads = ads;
  }

  return eligible_ads;
}

CreativeAdNotificationList EligibleAds::RemoveSeenAdsAndRoundRobinIfNeeded(
    const CreativeAdNotificationList& ads) const {
  const std::map<std::string, uint64_t> seen_ads =
      Client::Get()->GetSeenAdNotifications();

  CreativeAdNotificationList eligible_ads = FilterSeenAds(ads, seen_ads);

  if (eligible_ads.empty()) {
    BLOG(1, "All ads have been shown, so round robin");
    Client::Get()->ResetSeenAdNotifications(ads);
    eligible_ads = ads;
  }

  return eligible_ads;
}

CreativeAdNotificationList EligibleAds::FrequencyCap(
    const CreativeAdNotificationList& ads,
    const CreativeAdInfo& last_delivered_ad,
    const AdEventList& ad_events) const {
  CreativeAdNotificationList eligible_ads = ads;

  FrequencyCapping frequency_capping(subdivision_targeting_, ad_events);
  const auto iter = std::remove_if(eligible_ads.begin(), eligible_ads.end(),
      [&frequency_capping, &last_delivered_ad](CreativeAdInfo& ad) {
    return frequency_capping.ShouldExcludeAd(ad) ||
        ad.creative_instance_id == last_delivered_ad.creative_instance_id;
  });

  eligible_ads.erase(iter, eligible_ads.end());

  return eligible_ads;
}

}  // namespace ad_notifications
}  // namespace ads
