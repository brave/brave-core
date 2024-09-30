// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_generation_info.h"

#include <algorithm>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/span.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"

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

ArticleMetadata GetArticleMetadata(const mojom::FeedItemMetadataPtr& article,
                                   const std::vector<mojom::Signal*>& signals,
                                   std::vector<std::string> publisher_channels,
                                   const bool& discoverable) {
  // We should have at least one |Signal| from the |Publisher| for this source.
  CHECK(!signals.empty());

  const auto subscribed_weight = GetSubscribedWeight(article, signals);
  const double source_visits_min = features::kBraveNewsSourceVisitsMin.Get();
  const double source_visits_projected =
      source_visits_min + signals.at(0)->visit_weight * (1 - source_visits_min);
  const auto pop_recency = GetPopRecency(article);

  ArticleMetadata metadata;
  metadata.pop_recency = pop_recency;
  metadata.weighting =
      (source_visits_projected + subscribed_weight) * pop_recency;
  metadata.visited = signals.at(0)->visit_weight != 0;
  metadata.subscribed = subscribed_weight != 0,
  metadata.discoverable = discoverable;
  metadata.channels =
      base::flat_set<std::string>(std::move(publisher_channels));
  return metadata;
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

      auto channels = GetChannelsForPublisher(
          locale, publishers.at(article->data->publisher_id));
      ArticleInfo pair =
          std::tuple(article->data->Clone(),
                     GetArticleMetadata(article->data, article_signals,
                                        std::move(channels), discoverable));

      articles.push_back(std::move(pair));
    }
  }

  return articles;
}

}  // namespace

FeedGenerationInfo::FeedGenerationInfo(
    const SubscriptionsSnapshot& subscriptions,
    const std::string& locale,
    const FeedItems& feed_items,
    const Publishers& publishers,
    std::vector<std::string> channels,
    const Signals& signals,
    const std::vector<std::string>& suggested_publisher_ids,
    const TopicsResult& topics)
    : subscriptions_(subscriptions),
      locale_(locale),
      channels_(std::move(channels)),
      suggested_publisher_ids_(suggested_publisher_ids),
      suggested_publisher_ids_span_(base::make_span(suggested_publisher_ids_)) {
  this->feed_items_.reserve(feed_items.size());
  for (const auto& item : feed_items) {
    this->feed_items_.push_back(item->Clone());
  }

  this->publishers_.reserve(publishers.size());
  for (const auto& [id, publisher] : publishers) {
    this->publishers_[id] = publisher.Clone();
  }

  this->signals_.reserve(signals.size());
  for (const auto& [id, signal] : signals) {
    this->signals_[id] = signal->Clone();
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
    article_infos_ = brave_news::GetArticleInfos(locale_, feed_items_,
                                                 publishers_, signals_);
  }
  return article_infos_.value();
}

const std::vector<ContentGroup>&
FeedGenerationInfo::GetEligibleContentGroups() {
  if (!content_groups_) {
    GenerateAvailableCounts();

    std::vector<ContentGroup> content_groups;
    for (const auto& channel_id : channels_) {
      if (base::Contains(available_counts_, channel_id)) {
        content_groups.emplace_back(channel_id, true);
        DVLOG(1) << "Subscribed to channel: " << channel_id;
      } else {
        DVLOG(1)
            << "Subscribed to channel: " << channel_id
            << " which contains no articles (and thus, is not eligible as a "
               "group to pick content from)";
      }
    }

    for (const auto& [publisher_id, publisher] : publishers_) {
      if (publisher->user_enabled_status == mojom::UserEnabled::ENABLED ||
          publisher->type == mojom::PublisherType::DIRECT_SOURCE) {
        if (base::Contains(available_counts_, publisher_id)) {
          content_groups.emplace_back(publisher_id, false);
          DVLOG(1) << "Subscribed to publisher: " << publisher->publisher_name;
        } else {
          DVLOG(1) << "Subscribed to publisher: " << publisher->publisher_name
                   << " which has not articles available (and thus, isn't an "
                      "eligible content group)";
        }
      }
    }
    content_groups_ = std::move(content_groups);
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

  auto [article, metadata] = std::move(articles[index]);
  articles.erase(articles.begin() + index);

  ReduceCounts(article, metadata);

  return std::move(article);
}

void FeedGenerationInfo::GenerateAvailableCounts() {
  CHECK(available_counts_.empty());
  for (auto& [article, metadata] : GetArticleInfos()) {
    available_counts_[article->publisher_id]++;
    for (const auto& channel : metadata.channels) {
      available_counts_[channel]++;
    }
  }
}

void FeedGenerationInfo::ReduceCounts(const mojom::FeedItemMetadataPtr& article,
                                      const ArticleMetadata& meta) {
  // If we're not tracking content groups, we don't need to do this.
  if (!content_groups_.has_value()) {
    return;
  }

  // Decrease the publisher count for this article.
  std::vector<std::string> remove_content_groups;
  auto publisher_it = available_counts_.find(article->publisher_id);
  if (publisher_it != available_counts_.end()) {
    if (publisher_it->second <= 1) {
      remove_content_groups.emplace_back(article->publisher_id);
      available_counts_.erase(publisher_it);
    } else {
      publisher_it->second--;
    }
  }

  // Decrease the channel counts for this article.
  for (const auto& channel : meta.channels) {
    auto channel_it = available_counts_.find(channel);
    if (channel_it != available_counts_.end()) {
      if (channel_it->second <= 1) {
        remove_content_groups.emplace_back(channel);
        available_counts_.erase(channel_it);
      } else {
        channel_it->second--;
      }
    }
  }

  // Remove all the content groups that we've consumed all the articles from.
  for (const auto& to_remove : remove_content_groups) {
    DVLOG(1) << "Consumed the last article from " << to_remove
             << ". Removing it from the list of eligible content groups.";
    auto it = base::ranges::find_if(
        content_groups_.value(),
        [&to_remove](const auto& group) { return group.first == to_remove; });

    // We might not find a content_group for this entry because we might not be
    // directly subscribed (i.e. via a channel).
    if (it == content_groups_.value().end()) {
      continue;
    }

    content_groups_.value().erase(it);
  }
}

ArticleInfos GetArticleInfosForTesting(const std::string& locale,  // IN-TEST
                                       const FeedItems& feed_items,
                                       const Publishers& publishers,
                                       const Signals& signals) {
  return GetArticleInfos(locale, feed_items, publishers, signals);
}

}  // namespace brave_news
