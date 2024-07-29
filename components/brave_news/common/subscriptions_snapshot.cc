// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/subscriptions_snapshot.h"

#include <utility>

#include "base/containers/contains.h"

namespace brave_news {

SubscriptionsDiff::SubscriptionsDiff() = default;
SubscriptionsDiff::~SubscriptionsDiff() = default;
SubscriptionsDiff& SubscriptionsDiff::operator=(SubscriptionsDiff&&) = default;
SubscriptionsDiff::SubscriptionsDiff(SubscriptionsDiff&&) = default;

bool SubscriptionsDiff::IsEmpty() const {
  return changed.empty() && removed.empty();
}

SubscriptionsSnapshot::SubscriptionsSnapshot() = default;

SubscriptionsSnapshot::SubscriptionsSnapshot(
    base::flat_set<std::string> enabled_publishers,
    base::flat_set<std::string> disabled_publishers,
    std::vector<DirectFeed> direct_feeds,
    base::flat_map<std::string, std::vector<std::string>> channels)
    : enabled_publishers_(std::move(enabled_publishers)),
      disabled_publishers_(std::move(disabled_publishers)),
      direct_feeds_(std::move(direct_feeds)),
      channels_(std::move(channels)) {}

SubscriptionsSnapshot::SubscriptionsSnapshot(const SubscriptionsSnapshot&) =
    default;
SubscriptionsSnapshot& SubscriptionsSnapshot::operator=(
    const SubscriptionsSnapshot&) = default;

SubscriptionsSnapshot::SubscriptionsSnapshot(SubscriptionsSnapshot&&) = default;
SubscriptionsSnapshot& SubscriptionsSnapshot::operator=(
    SubscriptionsSnapshot&&) = default;

SubscriptionsSnapshot::~SubscriptionsSnapshot() = default;

std::vector<std::string> SubscriptionsSnapshot::GetChannelLocales() const {
  std::vector<std::string> locales;
  for (const auto& [key, value] : channels_) {
    locales.push_back(key);
  }
  return locales;
}

std::vector<std::string> SubscriptionsSnapshot::GetChannelLocales(
    const std::string& channel) const {
  std::vector<std::string> locales;

  for (const auto& [locale, locale_channels] : channels_) {
    if (base::Contains(locale_channels, channel)) {
      locales.push_back(locale);
    }
  }

  return locales;
}

bool SubscriptionsSnapshot::GetChannelSubscribed(
    const std::string& locale,
    const std::string& channel) const {
  for (const auto& [key, value] : channels_) {
    if (key != locale) {
      continue;
    }

    return base::Contains(value, channel);
  }
  return false;
}

SubscriptionsDiff SubscriptionsSnapshot::DiffPublishers(
    const SubscriptionsSnapshot& old) const {
  SubscriptionsDiff result;
  base::ranges::set_symmetric_difference(enabled_publishers_,
                                         old.enabled_publishers_,
                                         std::back_inserter(result.changed));
  base::ranges::set_symmetric_difference(disabled_publishers_,
                                         old.disabled_publishers_,
                                         std::back_inserter(result.changed));

  std::vector<std::string> direct_feeds_ids;
  base::ranges::transform(direct_feeds_, std::back_inserter(direct_feeds_ids),
                          &DirectFeed::id);
  std::vector<std::string> old_direct_feed_ids;
  base::ranges::transform(old.direct_feeds_,
                          std::back_inserter(old_direct_feed_ids),
                          &DirectFeed::id);

  base::flat_set<std::string> direct_feed_set(std::move(direct_feeds_ids));
  base::flat_set<std::string> old_direct_feed_set(
      std::move(old_direct_feed_ids));

  // New direct feeds should be added to the changed set.
  base::ranges::set_difference(direct_feed_set, old_direct_feed_set,
                               std::back_inserter(result.changed));

  // Removed direct feeds should be marked as removed.
  base::ranges::set_difference(old_direct_feed_set, direct_feed_set,
                               std::back_inserter(result.removed));
  return result;
}

SubscriptionsDiff SubscriptionsSnapshot::DiffChannels(
    const SubscriptionsSnapshot& other) const {
  SubscriptionsDiff result;
  std::vector<std::string> channel_ids;
  for (const auto& [locale, subscriptions] : channels_) {
    for (const auto& channel : subscriptions) {
      channel_ids.push_back(channel);
    }
  }

  std::vector<std::string> other_channel_ids;
  for (const auto& [locale, subscriptions] : other.channels_) {
    for (const auto& channel : subscriptions) {
      other_channel_ids.push_back(channel);
    }
  }
  base::flat_set<std::string> channels_set(std::move(channel_ids));
  base::flat_set<std::string> other_channels_set(std::move(other_channel_ids));

  base::ranges::set_symmetric_difference(channels_set, other_channels_set,
                                         std::back_inserter(result.changed));
  return result;
}

}  // namespace brave_news
