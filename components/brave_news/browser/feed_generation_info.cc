// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_generation_info.h"

#include <cstddef>
#include <string>

#include "base/containers/contains.h"
#include "base/containers/span.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"

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

  this->topics_.reserve(topics.size());
  for (const auto& topic : topics) {
    std::vector<api::topics::TopicArticle> articles;
    articles.reserve(topic.second.size());
    for (const auto& article : topic.second) {
      articles.push_back(article.Clone());
    }
    this->topics_.emplace_back(topic.first.Clone(), std::move(articles));
  }
  this->topics_span_ = base::make_span(this->topics_);
}

FeedGenerationInfo::FeedGenerationInfo(FeedGenerationInfo&&) = default;
FeedGenerationInfo& FeedGenerationInfo::operator=(FeedGenerationInfo&&) =
    default;
FeedGenerationInfo::~FeedGenerationInfo() = default;

ArticleInfos& FeedGenerationInfo::GetArticleInfos() {
  if (!article_infos_) {
    article_infos_ =
        brave_news::GetArticleInfos(locale, feed_items, publishers, signals);
  }
  return article_infos_.value();
}

mojom::FeedItemMetadataPtr FeedGenerationInfo::PickAndRemove(
    PickArticles picker) {
  auto maybe_index = picker.Run(GetArticleInfos());

  // There won't be an index if there were no eligible articles.
  if (!maybe_index.has_value()) {
    return nullptr;
  }

  CHECK(article_infos_.has_value());
  auto& articles = article_infos_.value();

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

size_t FeedGenerationInfo::GetArticleCount(
    const std::string& channel_or_publisher_id) {
  auto existing_it = available_counts_.find(channel_or_publisher_id);

  // If we've cached the value for this channel or publisher, return that.
  if (existing_it != available_counts_.end()) {
    return existing_it->second;
  }

  // Otherwise, calculate it:
  bool is_publisher = base::Contains(publishers, channel_or_publisher_id);
  size_t available = 0;
  for (const auto& [item, weight] : GetArticleInfos()) {
    if (is_publisher) {
      if (item->publisher_id == channel_or_publisher_id) {
        available++;
      }
      continue;
    }

    auto article_channels =
        GetChannelsForPublisher(locale, publishers.at(item->publisher_id));
    if (base::Contains(article_channels, channel_or_publisher_id)) {
      available++;
    }
  }

  available_counts_[channel_or_publisher_id] = available;
  return available;
}

}  // namespace brave_news
