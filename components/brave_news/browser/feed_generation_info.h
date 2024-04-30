// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_GENERATION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_GENERATION_INFO_H_

#include <glob.h>
#include <cstddef>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/browser/topics_fetcher.h"

namespace brave_news {

struct FeedGenerationInfo {
  std::string locale;
  FeedItems feed_items;
  Publishers publishers;
  std::vector<std::string> channels;
  Signals signals;
  std::vector<std::string> suggested_publisher_ids;
  TopicsResult topics;

  FeedGenerationInfo(const std::string& locale,
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
  mojom::FeedItemMetadataPtr PickAndRemove(PickArticles picker);
  size_t GetArticleCount(const std::string& channel_or_publisher_id);

 private:
  std::optional<ArticleInfos> article_infos_;
  base::flat_map<std::string, size_t> available_counts_;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_GENERATION_INFO_H_
