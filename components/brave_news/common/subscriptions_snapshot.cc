// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/subscriptions_snapshot.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"

namespace brave_news {

namespace {

// Calculate set difference A - B and append results to output vector
template <typename T>
void SetDifference(const absl::flat_hash_set<T>& a,
                   const absl::flat_hash_set<T>& b,
                   std::vector<T>& output) {
  for (const auto& item : a) {
    if (!b.contains(item)) {
      output.push_back(item);
    }
  }
}

// Calculate symmetric difference (A - B) ∪ (B - A) and append results to output
// vector
template <typename T>
void SymmetricDifference(const absl::flat_hash_set<T>& a,
                         const absl::flat_hash_set<T>& b,
                         std::vector<T>& output) {
  SetDifference(a, b, output);
  SetDifference(b, a, output);
}

}  // namespace

SubscriptionsDiff::SubscriptionsDiff() = default;
SubscriptionsDiff::~SubscriptionsDiff() = default;
SubscriptionsDiff& SubscriptionsDiff::operator=(SubscriptionsDiff&&) = default;
SubscriptionsDiff::SubscriptionsDiff(SubscriptionsDiff&&) = default;

bool SubscriptionsDiff::IsEmpty() const {
  return changed.empty() && removed.empty();
}

SubscriptionsSnapshot::SubscriptionsSnapshot() = default;

SubscriptionsSnapshot::SubscriptionsSnapshot(
    absl::flat_hash_set<std::string> enabled_publishers,
    absl::flat_hash_set<std::string> disabled_publishers,
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

std::vector<std::string> SubscriptionsSnapshot::GetChannelsFromAllLocales()
    const {
  std::vector<std::string> result;
  for (auto& [locale, channel] : channels_) {
    std::ranges::copy(channel, std::back_inserter(result));
  }
  return result;
}

SubscriptionsDiff SubscriptionsSnapshot::DiffPublishers(
    const SubscriptionsSnapshot& old) const {
  SubscriptionsDiff result;

  SymmetricDifference(enabled_publishers_, old.enabled_publishers_,
                      result.changed);
  SymmetricDifference(disabled_publishers_, old.disabled_publishers_,
                      result.changed);

  absl::flat_hash_set<std::string> direct_feed_ids;
  direct_feed_ids.reserve(direct_feeds_.size());
  for (const auto& feed : direct_feeds_) {
    direct_feed_ids.insert(feed.id);
  }

  absl::flat_hash_set<std::string> old_direct_feed_ids;
  old_direct_feed_ids.reserve(old.direct_feeds_.size());
  for (const auto& feed : old.direct_feeds_) {
    old_direct_feed_ids.insert(feed.id);
  }

  // New direct feeds should be added to the changed set.
  SetDifference(direct_feed_ids, old_direct_feed_ids, result.changed);

  // Removed direct feeds should be marked as removed.
  SetDifference(old_direct_feed_ids, direct_feed_ids, result.removed);

  return result;
}

SubscriptionsDiff SubscriptionsSnapshot::DiffChannels(
    const SubscriptionsSnapshot& other) const {
  SubscriptionsDiff result;

  absl::flat_hash_set<std::string> channel_ids;
  for (const auto& [locale, subscriptions] : channels_) {
    for (const auto& channel : subscriptions) {
      channel_ids.insert(channel);
    }
  }

  absl::flat_hash_set<std::string> other_channel_ids;
  for (const auto& [locale, subscriptions] : other.channels_) {
    for (const auto& channel : subscriptions) {
      other_channel_ids.insert(channel);
    }
  }

  SymmetricDifference(channel_ids, other_channel_ids, result.changed);

  return result;
}

}  // namespace brave_news
