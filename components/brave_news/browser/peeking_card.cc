// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/peeking_card.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <tuple>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/time/time.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"

namespace brave_news {

namespace {
using ItemScore = std::tuple</*index*/ size_t, /*score*/ double>;

constexpr size_t kMaxCandidates = 10;

constexpr double kDirectBoost = 15;
constexpr double kPublisherBoost = 10;
constexpr double kChannelBoost = 5;

constexpr double kOneHourMultiplier = 1.3;
constexpr double kOneDayMultiplier = 1.1;

constexpr double kMorningNewsBoost = 3;
constexpr double kEveningEntertainmentBoost = 3;

constexpr char kEntertainmentChannel[] = "Entertainment";
}  // namespace

std::optional<size_t> GetPeekingCard(const SubscriptionsSnapshot& subscriptions,
                                     const ArticleInfos& articles) {
  // Store now, so its consistent for everything.
  auto now = base::Time::Now();

  auto get_article =
      [&articles](size_t index) -> const mojom::FeedItemMetadataPtr& {
    return std::get<0>(articles[index]);
  };

  // Create sets for looking up whether articles are subscribed.
  base::flat_set<std::string> subscribed_channels(
      subscriptions.GetChannelLocales());
  std::set<std::string> direct_feed_publishers;
  for (const auto& direct : subscriptions.direct_feeds()) {
    direct_feed_publishers.insert(direct.id);
  }

  std::vector<ItemScore> candidates;
  for (size_t i = 0; i < articles.size(); ++i) {
    auto& [article, metadata] = articles[i];

    // Ignore disabled publishers completely - they should never be picked for
    // the peeking card.
    if (subscriptions.disabled_publishers().contains(article->publisher_id)) {
      continue;
    }

    double score = 0;

    // Boost direct feeds.
    if (direct_feed_publishers.contains(article->publisher_id)) {
      score += kDirectBoost;

      // Boost enabled publishers
    } else if (subscriptions.enabled_publishers().contains(
                   article->publisher_id)) {
      score += kPublisherBoost;

      // Boost enabled channels.
    } else if (std::ranges::any_of(article->channels, [&subscribed_channels](
                                                          const auto& channel) {
                 return base::Contains(subscribed_channels, channel);
               })) {
      score += kChannelBoost;
    }

    // Article has no score, so we can't do anything with it.
    if (score == 0) {
      continue;
    }

    // Apply a boost to recent articles.
    auto elapsed = now - article->publish_time;
    if (elapsed <= base::Hours(1)) {
      score *= kOneHourMultiplier;
    } else if (elapsed <= base::Days(1)) {
      score *= kOneDayMultiplier;
    }

    candidates.emplace_back(i, score);
  }

  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  for (auto& [index, score] : candidates) {
    const auto& article = get_article(index);
    // In the morning, weight news higher
    if (6 <= exploded.hour && exploded.hour < 10 &&
        base::Contains(article->channels, kTopNewsChannel)) {
      score += kMorningNewsBoost;

      // In the evening, weight entertainment higher
    } else if (17 <= exploded.hour && exploded.hour <= 22 &&
               base::Contains(article->channels, kEntertainmentChannel)) {
      score += kEveningEntertainmentBoost;
    }
  }

  base::ranges::sort(candidates, [](const auto& a, const auto& b) {
    return std::get<1>(a) > std::get<1>(b);
  });

  std::vector<ItemScore> final_candidates;
  std::map<std::string, size_t> seen_channels;
  size_t following_count = subscriptions.enabled_publishers().size() +
                           subscribed_channels.size() +
                           subscriptions.direct_feeds().size();
  // Make sure we can pick enough options to have at least |maxCandidates|
  // options in out finalCandidates.
  const auto channel_limit =
      following_count < kMaxCandidates ? (kMaxCandidates / following_count) : 1;
  for (auto& [index, score] : candidates) {
    if (final_candidates.size() > kMaxCandidates) {
      break;
    }

    const auto& article = get_article(index);

    // We don't want any channel to dominate our final candidates list.
    if (std::ranges::any_of(article->channels, [&seen_channels, channel_limit](
                                                   const auto& channel) {
          return seen_channels[channel] >= channel_limit;
        })) {
      continue;
    }

    final_candidates.emplace_back(index, score);
    for (const auto& channel : article->channels) {
      seen_channels[channel] += 1;
    }
  }

  if (final_candidates.size() == 0) {
    return std::nullopt;
  }

  auto [index, score] = PickRandom(final_candidates);
  return index;
}

}  // namespace brave_news
