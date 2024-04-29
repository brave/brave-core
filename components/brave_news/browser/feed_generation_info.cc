// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_generation_info.h"

namespace brave_news {

FeedGenerationInfo::FeedGenerationInfo(
    const std::string& locale,
    const FeedItems& feed_items,
    const Publishers& publishers,
    std::vector<std::string> channels,
    const Signals& signals,
    const std::vector<std::string>& suggested_publisher_ids,
    const TopicsResult& topics)
    : locale(locale),
      channels(std::move(channels)),
      suggested_publisher_ids(suggested_publisher_ids) {
  this->feed_items.reserve(feed_items.size());
  for (const auto& item : feed_items) {
    this->feed_items.push_back(item->Clone());
  }

  this->publishers.reserve(publishers.size());
  for (const auto& [id, publisher] : publishers) {
    this->publishers[id] = publisher.Clone();
  }

  this->signals.reserve(signals.size());
  for (const auto& [id, signal] : signals) {
    this->signals[id] = signal->Clone();
  }

  this->topics.reserve(topics.size());
  for (const auto& topic : topics) {
    std::vector<api::topics::TopicArticle> articles;
    articles.reserve(topic.second.size());
    for (const auto& article : topic.second) {
      articles.push_back(article.Clone());
    }
    this->topics.emplace_back(topic.first.Clone(), std::move(articles));
  }
}

FeedGenerationInfo::FeedGenerationInfo(FeedGenerationInfo&&) = default;
FeedGenerationInfo& FeedGenerationInfo::operator=(FeedGenerationInfo&&) =
    default;
FeedGenerationInfo::~FeedGenerationInfo() = default;
}  // namespace brave_news
