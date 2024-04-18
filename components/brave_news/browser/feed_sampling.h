// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_SAMPLING_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_SAMPLING_H_

#include <cstddef>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/rand_util.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"

namespace brave_news {

// An ArticleWeight has a few different components
struct ArticleWeight {
  // The pop_recency of the article. This is used for discover cards, where we
  // don't consider the subscription status or visit_weighting.
  double pop_recency = 0;

  // The complete weighting of the article, combining the pop_score,
  // visit_weighting & subscribed_weighting.
  double weighting = 0;

  // Whether the source which this article comes from has been visited. This
  // only considers Publishers, not Channels.
  bool visited = false;

  // Whether any sources/channels that could cause this article to be shown are
  // subscribed. At this point, disabled sources have already been filtered out.
  bool subscribed = false;
};

using ArticleInfo = std::tuple<mojom::FeedItemMetadataPtr, ArticleWeight>;
using ArticleInfos = std::vector<ArticleInfo>;

// Gets a weighting for a specific article. This determines how likely an
// article is to be chosen.
using GetWeighting =
    base::RepeatingCallback<double(const mojom::FeedItemMetadataPtr& metadata,
                                   const ArticleWeight& weight)>;

// PickArticles is a strategy used to pick articles (for example, taking the
// first article). Different feeds use different strategies for picking
// articles.
using PickArticles =
    base::RepeatingCallback<std::optional<size_t>(const ArticleInfos& infos)>;
using ContentGroup = std::pair<std::string, bool>;

template <typename T>
T PickRandom(const std::vector<T>& items) {
  CHECK(!items.empty());
  // Note: RandInt is inclusive, hence the minus 1
  return items[base::RandInt(0, items.size() - 1)];
}

// Sample across subscribed channels (direct and native) and publishers.
ContentGroup SampleContentGroup(
    const std::vector<ContentGroup>& eligible_content_groups);

std::vector<std::string> GetChannelsForPublisher(
    const std::string& locale,
    const mojom::PublisherPtr& publisher);

// Randomly true/false with equal probability.
bool TossCoin();

// This is a Box Muller transform for getting a normally distributed value
// between [0, 1)
// https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
double GetNormal();

// Returns a normally distributed value between min (inclusive) and max
// (exclusive).
int GetNormal(int min, int max);

// These functions are for deciding which article from a list to select. They
// should either:
// 1. Return a valid index into a list of articles.
// 2. Return -1 if there is no valid article to pick.
std::optional<size_t> PickFirstIndex(const ArticleInfos& articles);
std::optional<size_t> PickRouletteWithWeighting(const ArticleInfos& articles,
                                                GetWeighting get_weighting);
std::optional<size_t> PickRoulette(const ArticleInfos& articles);

mojom::FeedItemMetadataPtr PickAndRemove(ArticleInfos& articles,
                                         PickArticles picker);

// Picks an article with a probability article_weight/sum(article_weights).
mojom::FeedItemMetadataPtr PickRouletteAndRemove(ArticleInfos& articles);

mojom::FeedItemMetadataPtr PickDiscoveryArticleAndRemove(
    ArticleInfos& articles);

ArticleInfos GetArticleInfos(const std::string& locale,
                             const FeedItems& feed_items,
                             const Publishers& publishers,
                             const Signals& signals);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_SAMPLING_H_
