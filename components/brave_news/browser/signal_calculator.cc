// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/signal_calculator.h"

#include <iterator>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom-shared.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"

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

constexpr double kPopRecencyHalfLifeInHours = 18;

double GetPopRecency(const mojom::FeedItemMetadataPtr& data) {
  auto& publish_time = data->publish_time;

  // If the article doesn't have a score, default to 50
  double popularity = data->score == 0 ? 50 : data->score;

  // Double the score on articles from the last 5 hours.
  double multiplier = publish_time > base::Time::Now() - base::Hours(5) ? 2 : 1;

  auto dt = base::Time::Now() - publish_time;
  return multiplier * popularity *
         pow(0.5, dt.InHours() / kPopRecencyHalfLifeInHours);
}

bool IsPublisherSubscribed(mojom::UserEnabled user_enabled,
                           bool channel_subscribed) {
  return user_enabled == mojom::UserEnabled::ENABLED ||
         (channel_subscribed && user_enabled != mojom::UserEnabled::DISABLED);
}

}  // namespace

SignalCalculator::SignalCalculator(PublishersController& publishers_controller,
                                   ChannelsController& channels_controller,
                                   PrefService& prefs,
                                   history::HistoryService& history_service)
    : publishers_controller_(publishers_controller),
      channels_controller_(channels_controller),
      prefs_(prefs),
      history_service_(history_service) {}

SignalCalculator::~SignalCalculator() = default;

void SignalCalculator::GetSignals(const FeedItems& feed,
                                  SignalsCallback callback) {
  auto articles = GetArticles(feed);
  history::QueryOptions options;
  options.SetRecentDayRange(21);
  options.max_count = 2000;
  history_service_->QueryHistory(
      u"", options,
      base::BindOnce(&SignalCalculator::OnGotHistory,
                     weak_ptr_factory_.GetWeakPtr(), std::move(articles),
                     std::move(callback)),
      &task_tracker_);
}

void SignalCalculator::OnGotHistory(
    std::vector<mojom::FeedItemMetadataPtr> articles,
    SignalsCallback callback,
    history::QueryResults results) {
  const auto& locale = publishers_controller_->GetLastLocale();

  auto& publishers = publishers_controller_->GetLastPublishers();
  auto channels = channels_controller_->GetChannelsFromPublishers(
      publishers, &prefs_.get());
  base::flat_map<std::string, std::vector<std::string>> origin_visits;
  for (const auto& item : results) {
    auto host = item.url().host();
    origin_visits[host].push_back(item.url().spec());
  }

  // Start at one - it'll make the calculations very slightly off but it also
  // means we'll never divide by zero, and it will be consistent.
  uint32_t total_publisher_visits = 1;
  uint32_t total_channel_visits = 1;

  base::flat_map<std::string, std::vector<std::string>> publisher_visits;
  base::flat_map<std::string, std::vector<std::string>> channel_visits;

  for (auto& [publisher_id, publisher] : publishers) {
    auto host = publisher->site_url.host();
    auto history_it = origin_visits.find(host);
    if (history_it == origin_visits.end()) {
      publisher_visits[publisher_id] = {};
      continue;
    }

    base::ranges::copy(history_it->second,
                       std::back_inserter(publisher_visits[publisher_id]));
    total_publisher_visits += history_it->second.size();

    for (const auto& locale_info : publisher->locales) {
      if (locale_info->locale != locale) {
        continue;
      }

      for (const auto& channel : locale_info->channels) {
        total_channel_visits += history_it->second.size();
        base::ranges::copy(history_it->second,
                           std::back_inserter(channel_visits[channel]));
      }
      break;
    }
  }

  Signals signals;

  // Add article signals
  for (const auto& article : articles) {
    const auto& publisher = publishers.at(article->publisher_id);

    bool channel_subscribed = false;
    for (const auto& locale_info : publisher->locales) {
      for (const auto& channel : locale_info->channels) {
        if (channels_controller_->GetChannelSubscribed(locale_info->locale,
                                                       channel)) {
          channel_subscribed = true;
          break;
        }
      }
    }
    auto& visits = publisher_visits.at(article->publisher_id);
    signals[article->url.spec()] = {
        .subscribed = IsPublisherSubscribed(publisher->user_enabled_status,
                                            channel_subscribed),
        .visit_weight =
            visits.size() / static_cast<double>(total_publisher_visits),
        .pop_recency = GetPopRecency(article)};
  }

  // Add publisher signals
  for (const auto& it : publishers) {
    bool channel_subscribed = false;
    for (const auto& locale_info : it.second->locales) {
      for (const auto& channel : locale_info->channels) {
        if (channels_controller_->GetChannelSubscribed(locale_info->locale,
                                                       channel)) {
          channel_subscribed = true;
          break;
        }
      }
    }
    auto& visits = publisher_visits.at(it.first);
    signals[it.first] = {
        .subscribed = IsPublisherSubscribed(it.second->user_enabled_status,
                                            channel_subscribed),
        .visit_weight =
            visits.size() / static_cast<double>(total_publisher_visits),
        .pop_recency = 0};
  }

  // Add channel signals
  for (const auto& channel : channels) {
    auto it = channel_visits.find(channel.first);
    std::vector<std::string> visits =
        it == channel_visits.end() ? std::vector<std::string>{} : it->second;
    signals[channel.first] = {
        .subscribed =
            channels_controller_->GetChannelSubscribed(locale, channel.first),
        .visit_weight =
            visits.size() / static_cast<double>(total_channel_visits),
        .pop_recency = 0};
  }

  std::move(callback).Run(std::move(signals));
}

}  // namespace brave_news
