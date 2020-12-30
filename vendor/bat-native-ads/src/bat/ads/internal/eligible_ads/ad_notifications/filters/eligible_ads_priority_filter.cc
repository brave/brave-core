/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/ad_notifications/filters/eligible_ads_priority_filter.h"

#include <algorithm>
#include <map>
#include <utility>

#include "bat/ads/internal/logging.h"

namespace ads {

namespace {

using CreativeAdNotificationBucketPair =
    std::pair<unsigned int, CreativeAdNotificationList>;

using CreativeAdNotificationBucketMap =
    std::map<unsigned int, CreativeAdNotificationList>;

CreativeAdNotificationBucketMap BucketSortCreativeAdNotifications(
    const CreativeAdNotificationList& ads) {
  CreativeAdNotificationBucketMap buckets;

  for (const auto& ad : ads) {
    if (ad.priority == 0) {
      continue;
    }

    const auto iter = buckets.find(ad.priority);
    if (iter == buckets.end()) {
      buckets.insert({ad.priority, {ad}});
      continue;
    }

    iter->second.push_back(ad);
  }

  return buckets;
}

CreativeAdNotificationBucketPair GetHighestPriorityBucket(
    const CreativeAdNotificationBucketMap& buckets) {
  const auto iter = std::min_element(buckets.begin(), buckets.end(),
      [](const auto& lhs, const auto& rhs) {
    return lhs.first < rhs.first;
  });

  return *iter;
}

}  // namespace

EligibleAdsPriorityFilter::EligibleAdsPriorityFilter() = default;

EligibleAdsPriorityFilter::~EligibleAdsPriorityFilter() = default;

CreativeAdNotificationList EligibleAdsPriorityFilter::Apply(
    const CreativeAdNotificationList& ads) const {
  if (ads.empty()) {
    return {};
  }

  const CreativeAdNotificationBucketMap buckets =
      BucketSortCreativeAdNotifications(ads);
  if (buckets.empty()) {
    return {};
  }

  const CreativeAdNotificationBucketPair bucket =
      GetHighestPriorityBucket(buckets);

  const unsigned int priority = bucket.first;

  const CreativeAdNotificationList creative_ad_notifications = bucket.second;

  BLOG(2, creative_ad_notifications.size() << " eligible ads with a priority"
      " of " << priority);

  for (const auto& bucket : buckets) {
    if (bucket.first == priority) {
      continue;
    }

    BLOG(3, bucket.second.size() << " ads with a lower priority of "
        << bucket.first);
  }

  return creative_ad_notifications;
}

}  // namespace ads
