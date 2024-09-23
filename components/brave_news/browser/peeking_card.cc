// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/peeking_card.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/time/time.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/browser/topics_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "url/gurl.h"

namespace brave_news {

namespace {
using ItemScore = std::tuple</*index*/ size_t, /*score*/ double>;

constexpr size_t kMaxPeekingCardCandidates = 10;
// The percentage of the final candidates that are allowed to come from the same
// publisher.
constexpr double kMaxPublisherPercentOfCandidates = 0.2;

constexpr double kDirectBoost = 15;
constexpr double kPublisherBoost = 10;
constexpr double kChannelBoost = 5;

constexpr double kTopStoryMultiplier = 1.2;

constexpr double kOneHourMultiplier = 1.5;
constexpr double kThreeHoursMultiplier = 1.3;
constexpr double kSixHoursMultiplier = 1.2;
constexpr double kOneDayMultiplier = 1.1;

constexpr double kMorningNewsBoost = 3;
constexpr double kEveningEntertainmentBoost = 3;

constexpr double kMaxCandidatesScorePercentCutoff = 0.7;

constexpr char kEntertainmentChannel[] = "Entertainment";
}  // namespace

base::flat_set<std::string> GetTopStoryUrls(
    const base::span<TopicAndArticles>& topics) {
  std::vector<std::string> urls;
  for (auto& [topic, articles] : topics) {
    base::ranges::transform(articles, std::back_inserter(urls),
                            [](const auto& article) { return article.url; });
  }
  return base::flat_set<std::string>(urls);
}

std::optional<size_t> PickPeekingCardWithMax(
    SubscriptionsSnapshot subscriptions,
    base::flat_set<std::string> top_story_urls,
    const ArticleInfos& articles,
    size_t max_candidates) {
  // Store now, so it's consistent for everything.
  auto now = base::Time::Now();

  auto get_article =
      [&articles](size_t index) -> const mojom::FeedItemMetadataPtr& {
    return std::get<0>(articles[index]);
  };

  // Create sets for looking up whether articles are subscribed.
  base::flat_set<std::string> subscribed_channels(
      subscriptions.GetChannelsFromAllLocales());
  std::set<std::string> direct_feed_publishers;
  for (const auto& direct : subscriptions.direct_feeds()) {
    direct_feed_publishers.insert(direct.id);
  }

  size_t following_count = subscriptions.enabled_publishers().size() +
                           subscribed_channels.size() +
                           subscriptions.direct_feeds().size();
  if (following_count == 0) {
    return std::nullopt;
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

    if (base::Contains(top_story_urls, article->url.spec())) {
      score *= kTopStoryMultiplier;
    }

    // Apply a boost to recent articles.
    auto elapsed = now - article->publish_time;
    if (elapsed <= base::Hours(1)) {
      score *= kOneHourMultiplier;
    } else if (elapsed <= base::Hours(3)) {
      score *= kThreeHoursMultiplier;
    } else if (elapsed <= base::Hours(6)) {
      score *= kSixHoursMultiplier;
    } else if (elapsed <= base::Days(1)) {
      score *= kOneDayMultiplier;
    } else {
      // Decay for a week - after a week there probably isn't much difference.
      // (a half life of 1.1 days does nicely here)
      score *= std::max(0.1, pow(0.5, elapsed.InDays() / 1.1));
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

  base::ranges::sort(candidates, [get_article](const auto& a, const auto& b) {
    const auto& [a_index, a_score] = a;
    const auto& [b_index, b_score] = b;
    if (a_score != b_score) {
      return a_score > b_score;
    }

    const auto& a_article = get_article(a_index);
    const auto& b_article = get_article(b_index);

    return a_article->publish_time > b_article->publish_time;
  });

  if (candidates.empty()) {
    return std::nullopt;
  }

  std::vector<ItemScore> final_candidates;
  std::map<std::string, size_t> seen_publishers;

  // Limit each publisher to a percentage of the final candidates (i.e. no more
  // than 20% of the candidates should come from one source).
  const auto publisher_limit = static_cast<int>(
      kMaxPeekingCardCandidates * kMaxPublisherPercentOfCandidates);
  // This is the minimum score that we'll consider candidates at;
  const auto min_score =
      kMaxCandidatesScorePercentCutoff * std::get<1>(candidates.front());

  for (auto& [index, score] : candidates) {
    if (final_candidates.size() >= max_candidates || score < min_score) {
      break;
    }

    const auto& article = get_article(index);

    if (seen_publishers[article->publisher_id] >= publisher_limit) {
      continue;
    }

    seen_publishers[article->publisher_id]++;

    final_candidates.emplace_back(index, score);
  }

  if (final_candidates.empty()) {
    return std::nullopt;
  }

  auto [index, score] = PickRandom(final_candidates);
  return index;
}

std::optional<size_t> PickPeekingCard(
    SubscriptionsSnapshot subscriptions,
    base::flat_set<std::string> top_story_urls,
    const ArticleInfos& articles) {
  return PickPeekingCardWithMax(std::move(subscriptions),
                                std::move(top_story_urls), articles,
                                kMaxPeekingCardCandidates);
}

}  // namespace brave_news
