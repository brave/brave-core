// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/signal_calculator.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "brave/components/brave_news/browser/background_history_querier.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"

namespace brave_news {

namespace {

std::vector<mojom::FeedItemMetadataPtr> GetArticles(const FeedItems& feed) {
  std::vector<mojom::FeedItemMetadataPtr> articles;
  for (const auto& item : feed) {
    if (item->is_article()) {
      articles.push_back(item->get_article()->data->Clone());
    }
  }
  return articles;
}

}  // namespace

SignalCalculator::SignalCalculator(PublishersController& publishers_controller,
                                   ChannelsController& channels_controller,
                                   BackgroundHistoryQuerier& history_querier)
    : publishers_controller_(publishers_controller),
      channels_controller_(channels_controller),
      history_querier_(history_querier) {}

SignalCalculator::~SignalCalculator() = default;

void SignalCalculator::GetSignals(const SubscriptionsSnapshot& subscriptions,
                                  const FeedItems& feed,
                                  SignalsCallback callback) {
  auto articles = GetArticles(feed);
  history_querier_->Run(base::BindOnce(
      &SignalCalculator::OnGotHistory, weak_ptr_factory_.GetWeakPtr(),
      subscriptions, std::move(articles), std::move(callback)));
}

void SignalCalculator::OnGotHistory(
    const SubscriptionsSnapshot& subscriptions,
    std::vector<mojom::FeedItemMetadataPtr> articles,
    SignalsCallback callback,
    history::QueryResults results) {
  const auto& locale = publishers_controller_->GetLastLocale();

  const auto& publishers = publishers_controller_->last_publishers();
  const auto& channels = channels_controller_->GetChannelsFromPublishers(
      publishers, subscriptions);

  // Work out how many articles we have in each publisher/channel. We'll use
  // these values to normalize the boost we apply to articles within those
  // publishers/channels so we don't overwhelm the user with articles from
  // certain areas.
  absl::flat_hash_map<std::string, uint32_t> article_counts;
  for (const auto& article : articles) {
    auto* publisher = base::FindPtrOrNull(publishers, article->publisher_id);
    if (!publisher) {
      continue;
    }

    article_counts[article->publisher_id]++;
    for (const auto& locale_info : publisher->locales) {
      if (locale_info->locale != locale) {
        continue;
      }
      for (const auto& channel : locale_info->channels) {
        article_counts[channel]++;
      }
    }
  }

  absl::flat_hash_map<std::string, std::vector<std::string>> origin_visits;
  for (const auto& item : results) {
    auto host = item.url().host();
    origin_visits[host].push_back(item.url().spec());
  }

  // Start at one - it'll make the calculations very slightly off but it also
  // means we'll never divide by zero, and it will be consistent.
  uint32_t total_publisher_visits = 1;
  uint32_t total_channel_visits = 1;

  absl::flat_hash_map<std::string, std::vector<std::string>> publisher_visits;
  absl::flat_hash_map<std::string, std::vector<std::string>> channel_visits;

  for (auto& [publisher_id, publisher] : publishers) {
    auto host = publisher->site_url.host();

    // Direct feeds don't get a site_url, just a source, so fallback to that.
    if (host.empty()) {
      host = publisher->feed_source.host();
    }

    auto* visits = base::FindOrNull(origin_visits, host);
    if (!visits) {
      publisher_visits[publisher_id] = {};
      continue;
    }

    std::ranges::copy(*visits,
                      std::back_inserter(publisher_visits[publisher_id]));
    total_publisher_visits += visits->size();

    for (const auto& locale_info : publisher->locales) {
      if (locale_info->locale != locale) {
        continue;
      }

      for (const auto& channel : locale_info->channels) {
        total_channel_visits += visits->size();
        std::ranges::copy(*visits, std::back_inserter(channel_visits[channel]));
      }
      break;
    }
  }

  std::vector<std::pair<std::string, Signal>> signals_vec;

  // Add publisher signals
  for (const auto& [id, publisher] : publishers) {
    const auto& visits = publisher_visits.at(publisher->publisher_id);
    auto disabled =
        publisher->user_enabled_status == mojom::UserEnabled::DISABLED;
    signals_vec.emplace_back(
        id, mojom::Signal::New(
                disabled, GetSubscribedWeight(publisher),
                visits.size() / static_cast<double>(total_publisher_visits),
                article_counts[id]));
  }

  // Add channel signals
  for (const auto& channel : channels) {
    auto* channel_visit_vec = base::FindOrNull(channel_visits, channel.first);
    auto visit_count = channel_visit_vec ? channel_visit_vec->size() : 0;
    signals_vec.emplace_back(
        channel.first,
        mojom::Signal::New(
            /*disabled=*/false,
            subscriptions.GetChannelSubscribed(locale, channel.first)
                ? features::kBraveNewsChannelSubscribedBoost.Get()
                : 0,
            visit_count / static_cast<double>(total_channel_visits),
            article_counts[channel.first]));
  }

  Signals signals(std::move(signals_vec));
  std::move(callback).Run(std::move(signals));
}

double SignalCalculator::GetSubscribedWeight(
    const mojom::PublisherPtr& publisher) {
  auto enabled = publisher->user_enabled_status;
  // Disabled sources should never show up in the feed
  if (enabled == mojom::UserEnabled::DISABLED) {
    return 0;
  }

  return publisher->type == mojom::PublisherType::DIRECT_SOURCE ||
                 enabled == mojom::UserEnabled::ENABLED
             ? features::kBraveNewsSourceSubscribedBoost.Get()
             : 0;
}

}  // namespace brave_news
