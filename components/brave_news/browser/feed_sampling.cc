// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_sampling.h"

#include <algorithm>
#include <numeric>
#include <optional>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/rand_util.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/features.h"

namespace brave_news {

namespace {

double GetSubscribedWeight(const mojom::FeedItemMetadataPtr& article,
                           const std::vector<mojom::Signal*>& signals) {
  return std::reduce(
      signals.begin(), signals.end(), 0.0, [](double prev, const auto* signal) {
        return prev + signal->subscribed_weight / signal->article_count;
      });
}

double GetPopRecency(const mojom::FeedItemMetadataPtr& article) {
  const double pop_recency_half_life_in_hours =
      features::kBraveNewsPopScoreHalfLife.Get();

  auto& publish_time = article->publish_time;

  double popularity = std::min(article->pop_score, 100.0) / 100.0 +
                      features::kBraveNewsPopScoreMin.Get();
  double multiplier = publish_time > base::Time::Now() - base::Hours(5) ? 2 : 1;
  auto dt = base::Time::Now() - publish_time;

  return multiplier * popularity *
         pow(0.5, dt.InHours() / pop_recency_half_life_in_hours);
}

// Gets all relevant signals for an article.
// **Note:** Importantly, this function returns the Signal from the publisher
// first, and |GetArticleWeight| depends on this to determine whether the
// Publisher has been visited.
std::vector<mojom::Signal*> GetSignals(
    const std::string& locale,
    const mojom::FeedItemMetadataPtr& article,
    const Publishers& publishers,
    const Signals& signals) {
  std::vector<mojom::Signal*> result;
  auto it = signals.find(article->publisher_id);
  if (it != signals.end()) {
    result.push_back(it->second.get());
  }

  auto publisher_it = publishers.find(article->publisher_id);
  if (publisher_it == publishers.end()) {
    return result;
  }

  for (const auto& locale_info : publisher_it->second->locales) {
    if (locale_info->locale != locale) {
      continue;
    }
    for (const auto& channel : locale_info->channels) {
      auto signal_it = signals.find(channel);
      if (signal_it == signals.end()) {
        continue;
      }
      result.push_back(signal_it->second.get());
    }
  }
  return result;
}

ArticleWeight GetArticleWeight(const mojom::FeedItemMetadataPtr& article,
                               const std::vector<mojom::Signal*>& signals,
                               const bool& discoverable) {
  // We should have at least one |Signal| from the |Publisher| for this source.
  CHECK(!signals.empty());

  const auto subscribed_weight = GetSubscribedWeight(article, signals);
  const double source_visits_min = features::kBraveNewsSourceVisitsMin.Get();
  const double source_visits_projected =
      source_visits_min + signals.at(0)->visit_weight * (1 - source_visits_min);
  const auto pop_recency = GetPopRecency(article);

  return {
      .pop_recency = pop_recency,
      .weighting = (source_visits_projected + subscribed_weight) * pop_recency,
      // Note: GetArticleWeight returns the Signal for the Publisher first, and
      // we use that to determine whether this Publisher has ever been visited.
      .visited = signals.at(0)->visit_weight != 0,
      .subscribed = subscribed_weight != 0,
      .discoverable = discoverable};
}

}  // namespace

ContentGroup SampleContentGroup(
    const std::vector<ContentGroup>& eligible_content_groups) {
  ContentGroup sampled_content_group;
  if (eligible_content_groups.empty()) {
    return sampled_content_group;
  }

  return PickRandom<ContentGroup>(eligible_content_groups);
}

bool TossCoin() {
  return base::RandDouble() < 0.5;
}

double GetNormal() {
  double u;
  double v;

  do {
    u = base::RandDouble();
  } while (u == 0);
  do {
    v = base::RandDouble();
  } while (v == 0);

  double result = sqrt(-2.0 * log(u)) * cos(2 * 3.1415 * v);
  result = result / 10 + 0.5;

  // Resample if outside the [0, 1] range
  if (result > 1 || result < 0) {
    return GetNormal();
  }

  return result;
}

int GetNormal(int min, int max) {
  return min + floor((max - min) * GetNormal());
}

std::optional<size_t> PickFirstIndex(const ArticleInfos& articles) {
  return articles.empty() ? std::nullopt : std::make_optional(0);
}

std::optional<size_t> PickRouletteWithWeighting(const ArticleInfos& articles,
                                                GetWeighting get_weighting) {
  std::vector<double> weights;
  base::ranges::transform(articles, std::back_inserter(weights),
                          [&get_weighting](const auto& article_info) {
                            return get_weighting.Run(std::get<0>(article_info),
                                                     std::get<1>(article_info));
                          });

  // None of the items are eligible to be picked.
  const auto total_weight =
      std::accumulate(weights.begin(), weights.end(), 0.0);
  if (total_weight == 0) {
    return std::nullopt;
  }

  double picked_value = base::RandDouble() * total_weight;
  double current_weight = 0;

  for (size_t i = 0; i < weights.size(); ++i) {
    current_weight += weights[i];
    if (current_weight >= picked_value) {
      return i;
    }
  }

  return std::nullopt;
}

std::optional<size_t> PickRoulette(const ArticleInfos& articles) {
  return PickRouletteWithWeighting(
      articles,
      base::BindRepeating([](const mojom::FeedItemMetadataPtr& metadata,
                             const ArticleWeight& weight) {
        return weight.subscribed ? weight.weighting : 0;
      }));
}

mojom::FeedItemMetadataPtr PickAndRemove(ArticleInfos& articles,
                                         PickArticles picker) {
  auto maybe_index = picker.Run(articles);

  // There won't be an index if there were no eligible articles.
  if (!maybe_index.has_value()) {
    return nullptr;
  }

  auto index = maybe_index.value();
  if (index >= articles.size()) {
    DCHECK(false) << "|index| should never be outside the bounds of |articles| "
                     "(index: "
                  << index << ", articles.size(): " << articles.size();
    return nullptr;
  }

  auto [article, weight] = std::move(articles[index]);
  articles.erase(articles.begin() + index);

  return std::move(article);
}

// Picking a discovery article works the same way as as a normal roulette
// selection, but we only consider articles that:
// 1. The user hasn't subscribed to.
// 2. **AND** The user hasn't visited.
mojom::FeedItemMetadataPtr PickDiscoveryArticleAndRemove(
    ArticleInfos& articles) {
  PickArticles pick = base::BindRepeating([](const ArticleInfos& articles) {
    return PickRouletteWithWeighting(
        articles,
        base::BindRepeating([](const mojom::FeedItemMetadataPtr& metadata,
                               const ArticleWeight& weight) {
          if (!weight.discoverable) {
            return 0.0;
          }

          if (weight.subscribed) {
            return 0.0;
          }
          return weight.pop_recency;
        }));
  });
  return PickAndRemove(articles, pick);
}

ArticleInfos GetArticleInfos(const std::string& locale,
                             const FeedItems& feed_items,
                             const Publishers& publishers,
                             const Signals& signals) {
  ArticleInfos articles;
  base::flat_set<GURL> seen_articles;
  base::flat_set<std::string> non_discoverable_publishers;

  for (const auto& [publisher_id, publisher] : publishers) {
    auto channels = GetChannelsForPublisher(locale, publisher);
    if (base::ranges::any_of(kSensitiveChannels,
                             [&](const std::string& channel) {
                               return base::Contains(channels, channel);
                             })) {
      non_discoverable_publishers.insert(publisher_id);
    }
  }

  for (const auto& item : feed_items) {
    if (item.is_null()) {
      continue;
    }
    if (item->is_article()) {
      // Because we download feeds from multiple locales, it's possible there
      // will be duplicate articles, which we should filter out.
      if (seen_articles.contains(item->get_article()->data->url)) {
        continue;
      }
      auto& article = item->get_article();

      seen_articles.insert(article->data->url);

      auto article_signals =
          GetSignals(locale, article->data, publishers, signals);

      // If we don't have any signals for this article, or the source this
      // article comes from has been disabled then filter it out.
      if (article_signals.empty() ||
          base::ranges::any_of(article_signals, [](const auto* signal) {
            return signal->disabled;
          })) {
        continue;
      }

      const bool discoverable = !base::Contains(non_discoverable_publishers,
                                                article->data->publisher_id);

      ArticleInfo pair = std::tuple(
          article->data->Clone(),
          GetArticleWeight(article->data, article_signals, discoverable));

      articles.push_back(std::move(pair));
    }
  }

  return articles;
}

}  // namespace brave_news
