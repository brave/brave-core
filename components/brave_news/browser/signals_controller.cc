// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/signals_controller.h"

#include <cstdint>
#include <iterator>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_controller.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom-shared.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "components/history/core/browser/history_types.h"
#include "components/prefs/pref_service.h"

namespace brave_news {

namespace {

std::vector<mojom::FeedItemMetadataPtr> GetArticles(FeedItems feed) {
  std::vector<mojom::FeedItemMetadataPtr> articles;
  for (const auto& item : feed) {
    if (item->is_article()) {
      articles.push_back(std::move(item->get_article()->data));
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

}  // namespace

SignalsController::SignalsController(
    PublishersController* publishers_controller,
    ChannelsController* channels_controller,
    RawFeedController* feed_controller,
    PrefService* prefs,
    history::HistoryService* history_service)
    : publishers_controller_(publishers_controller),
      channels_controller_(channels_controller),
      feed_controller_(feed_controller),
      prefs_(prefs),
      history_service_(history_service) {}

SignalsController::~SignalsController() = default;

void SignalsController::GetSignals(SignalsCallback callback) {
  feed_controller_->GetOrFetchFeed(base::BindOnce(
      [](SignalsController* controller, SignalsCallback callback,
         FeedItems feed) {
        auto articles = GetArticles(std::move(feed));
        history::QueryOptions options;
        options.SetRecentDayRange(21);
        options.max_count = 2000;
        controller->history_service_->QueryHistory(
            u"", options,
            base::BindOnce(&SignalsController::OnGotHistory,
                           base::Unretained(controller), std::move(articles),
                           std::move(callback)),
            &controller->task_tracker_);
      },
      base::Unretained(this), std::move(callback)));
}

void SignalsController::OnGotHistory(
    std::vector<mojom::FeedItemMetadataPtr> articles,
    SignalsCallback callback,
    history::QueryResults results) {
  // TODO(fallaciousreasoning): Actually get the locale.
  const char locale[] = "en_US";

  auto& publishers = publishers_controller_->GetLastPublishers();
  auto channels =
      channels_controller_->GetChannelsFromPublishers(publishers, prefs_);
  base::flat_map<std::string, std::vector<std::string>> origin_visits;
  for (const auto& item : results) {
    auto host = item.url().host();
    origin_visits[host].push_back(item.url().spec());
  }

  // Start at one - it'll make the calculations very slightly off but it also
  // means we'll never divide by zero.
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
    signals[article->url.spec()] = mojom::Signal::New(
        publisher->user_enabled_status == mojom::UserEnabled::DISABLED,
        channel_subscribed, -1,
        publisher->user_enabled_status == mojom::UserEnabled::ENABLED,
        visits.size() / static_cast<double>(total_publisher_visits), visits,
        GetPopRecency(article));
  }

  for (const auto& it : publishers) {
    auto& visits = publisher_visits.at(it.first);
    signals[it.first] = mojom::Signal::New(
        it.second->user_enabled_status == mojom::UserEnabled::DISABLED, false,
        -1, it.second->user_enabled_status == mojom::UserEnabled::ENABLED,
        visits.size() / static_cast<double>(total_publisher_visits), visits, 0);
  }

  for (const auto& channel : channels) {
    auto it = channel_visits.find(channel.first);
    std::vector<std::string> visits =
        it == channel_visits.end() ? std::vector<std::string>{} : it->second;
    signals[channel.first] = mojom::Signal::New(
        false,
        channels_controller_->GetChannelSubscribed(locale, channel.first),
        visits.size() / static_cast<double>(total_channel_visits), false, 0,
        visits, 0);
  }

  std::move(callback).Run(std::move(signals));
}

}  // namespace brave_news
