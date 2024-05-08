// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_generation_info.h"

#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/span.h"
#include "base/ranges/algorithm.h"
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

const ArticleInfos& FeedGenerationInfo::GetArticleInfos() {
  if (!article_infos_) {
    article_infos_ =
        brave_news::GetArticleInfos(locale, feed_items, publishers, signals);
  }
  return article_infos_.value();
}

const std::vector<ContentGroup>&
FeedGenerationInfo::GetEligibleContentGroups() {
  if (!content_groups_) {
    content_groups_ = GenerateContentGroups();
  }
  return content_groups_.value();
}

std::vector<std::string> FeedGenerationInfo::EligibleChannels() {
  std::vector<std::string> eligible_channels;
  for (auto& [group, is_channel] : GetEligibleContentGroups()) {
    if (!is_channel) {
      continue;
    }
    eligible_channels.push_back(group);
  }
  return eligible_channels;
}

mojom::FeedItemMetadataPtr FeedGenerationInfo::PickAndConsume(
    PickArticles picker) {
  CHECK(article_infos_.has_value());
  auto& articles = article_infos_.value();

  auto maybe_index = picker.Run(article_infos_.value());
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

void FeedGenerationInfo::GenerateAvailableCounts() {
  CHECK(available_counts_.empty());
  for (auto& [article, weight] : GetArticleInfos()) {
    available_counts_[article->publisher_id]++;
    for (const auto& channel : weight.channels) {
      available_counts_[channel]++;
    }
  }
}

std::vector<ContentGroup> FeedGenerationInfo::GenerateContentGroups() {
  GenerateAvailableCounts();

  std::vector<ContentGroup> eligible_content_groups;
  for (const auto& channel_id : channels) {
    if (base::Contains(available_counts_, channel_id)) {
      eligible_content_groups.emplace_back(channel_id, true);
      DVLOG(1) << "Subscribed to channel: " << channel_id;
    } else {
      DVLOG(1) << "Subscribed to channel: " << channel_id
               << " which contains no articles (and thus, is not eligible as a "
                  "group to pick content from)";
    }
  }

  for (const auto& [publisher_id, publisher] : publishers) {
    if (publisher->user_enabled_status == mojom::UserEnabled::ENABLED ||
        publisher->type == mojom::PublisherType::DIRECT_SOURCE) {
      if (base::Contains(available_counts_, publisher_id)) {
        eligible_content_groups.emplace_back(publisher_id, false);
        DVLOG(1) << "Subscribed to publisher: " << publisher->publisher_name;
      } else {
        DVLOG(1) << "Subscribed to publisher: " << publisher->publisher_name
                 << " which has not articles available (and thus, isn't an "
                    "eligible content group)";
      }
    }
  }

  return eligible_content_groups;
}

void FeedGenerationInfo::ReduceCounts(const mojom::FeedItemMetadataPtr& article,
                                      const ArticleWeight& weight) {
  // If we're not tracking content groups, we don't need to do this.
  if (!content_groups_.has_value()) {
    return;
  }

  // Decrease the publisher count for this article.
  std::vector<std::string> remove_content_groups;
  auto publisher_it = available_counts_.find(article->publisher_id);
  if (publisher_it != available_counts_.end()) {
    if (publisher_it->second <= 1) {
      available_counts_.erase(publisher_it);
      remove_content_groups.emplace_back(article->publisher_id);
    } else {
      publisher_it->second--;
    }
  }

  // Decrease the channel counts for this article.
  for (const auto& channel : weight.channels) {
    auto channel_it = available_counts_.find(channel);
    if (channel_it != available_counts_.end()) {
      remove_content_groups.emplace_back(channel);
      available_counts_.erase(channel_it);
    } else {
      channel_it->second--;
    }
  }

  // Remove all the content groups that we've consumed all the articles from.
  for (const auto& to_remove : remove_content_groups) {
    content_groups_.value().erase(base::ranges::find_if(
        content_groups_.value(),
        [&to_remove](const auto& group) { return group.first == to_remove; }));
  }
}

}  // namespace brave_news
