// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_GENERATION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_GENERATION_INFO_H_

#include <cstddef>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/browser/topics_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"

namespace brave_news {

class FeedGenerationInfo {
 public:
  FeedGenerationInfo(const SubscriptionsSnapshot& subscriptions,
                     const std::string& locale,
                     const FeedItems& feed_items,
                     const Publishers& publishers,
                     std::vector<std::string> channels,
                     const Signals& signals,
                     const std::vector<std::string>& suggested_publisher_ids,
                     const TopicsResult& topics);

  FeedGenerationInfo(const FeedGenerationInfo&) = delete;
  FeedGenerationInfo& operator=(const FeedGenerationInfo&) = delete;
  FeedGenerationInfo(FeedGenerationInfo&&);
  FeedGenerationInfo& operator=(FeedGenerationInfo&&);
  ~FeedGenerationInfo();

  const ArticleInfos& GetArticleInfos();

  // Gets a list of content groups that:
  // 1. The user is subscribed to
  // 2. Contain at least one article.
  const std::vector<ContentGroup>& GetEligibleContentGroups();

  // Get a list of subscribed channels containing at least one article.
  std::vector<std::string> EligibleChannels();

  // Picks an article and decreases publisher/channel counts appropriately to
  // maintain the list of content groups.
  mojom::FeedItemMetadataPtr PickAndConsume(PickArticles picker);

  const SubscriptionsSnapshot& subscriptions() { return subscriptions_; }
  const std::string locale() { return locale_; }
  const Publishers& publishers() { return publishers_; }

  // Returns a pointer to the set of raw feed_items
  FeedItems& raw_feed_items() { return feed_items_; }

  // A modifiable span of the available topics.
  base::span<TopicAndArticles>& topics() { return topics_span_; }

  // A modifiable span
  base::span<std::string>& suggested_publisher_ids() {
    return suggested_publisher_ids_span_;
  }

 private:
  friend class BraveNewsFeedGenerationInfoTest;

  void GenerateAvailableCounts();
  void ReduceCounts(const mojom::FeedItemMetadataPtr& article,
                    const ArticleMetadata& meta);

  SubscriptionsSnapshot subscriptions_;

  std::string locale_;
  std::vector<std::string> channels_;
  Publishers publishers_;

  FeedItems feed_items_;

  std::vector<std::string> suggested_publisher_ids_;
  base::span<std::string> suggested_publisher_ids_span_;

  Signals signals_;

  base::span<TopicAndArticles> topics_span_;
  TopicsResult topics_;

  std::optional<ArticleInfos> article_infos_;
  std::optional<std::vector<ContentGroup>> content_groups_;
  base::flat_map<std::string, size_t> available_counts_;
};

ArticleInfos GetArticleInfosForTesting(const std::string& locale,
                                       const FeedItems& feed_items,
                                       const Publishers& publishers,
                                       const Signals& signals);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_GENERATION_INFO_H_
