// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PEEKING_CARD_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PEEKING_CARD_H_

#include <string>

#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/browser/topics_fetcher.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace brave_news {

// Iterate over all topics and articles and return a set of unique URLs.
absl::flat_hash_set<std::string> GetTopStoryUrls(
    const base::span<TopicAndArticles>& topics);

std::optional<size_t> PickPeekingCardWithMax(
    SubscriptionsSnapshot subscriptions,
    const absl::flat_hash_set<std::string>& top_story_urls,
    const ArticleInfos& articles,
    size_t max_candidates);

std::optional<size_t> PickPeekingCard(
    SubscriptionsSnapshot subscriptions,
    const absl::flat_hash_set<std::string>& top_story_urls,
    const ArticleInfos& articles);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PEEKING_CARD_H_
