// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_controller.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/combined_feed_parsing.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/feed_building.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/locales_helper.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_news/common/brave_news.mojom-shared.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_private_cdn/headers.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

FeedController::FeedController(
    PublishersController* publishers_controller,
    DirectFeedController* direct_feed_controller,
    ChannelsController* channels_controller,
    history::HistoryService* history_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs)
    : prefs_(prefs),
      publishers_controller_(publishers_controller),
      direct_feed_controller_(direct_feed_controller),
      channels_controller_(channels_controller),
      history_service_(history_service),
      feed_fetcher_(*publishers_controller,
                    *channels_controller,
                    url_loader_factory),
      on_current_update_complete_(new base::OneShotEvent()) {
  publishers_observation_.Observe(publishers_controller);
}

FeedController::~FeedController() = default;

void FeedController::DoesFeedVersionDiffer(
    const std::string& matching_hash,
    mojom::BraveNewsController::IsFeedUpdateAvailableCallback callback) {
  GetOrFetchFeed(base::BindOnce(
      [](FeedController* controller, std::string matching_hash,
         mojom::BraveNewsController::IsFeedUpdateAvailableCallback callback) {
        VLOG(1) << "DoesFeedVersionMatch? " << matching_hash << " "
                << controller->current_feed_.hash;
        std::move(callback).Run(matching_hash !=
                                controller->current_feed_.hash);
      },
      base::Unretained(this), matching_hash, std::move(callback)));
}

void FeedController::AddListener(
    mojo::PendingRemote<mojom::FeedListener> listener) {
  listeners_.Add(std::move(listener));
}

void FeedController::GetOrFetchFeed(GetFeedCallback callback) {
  GetOrFetchFeed(base::BindOnce(
      [](FeedController* controller, GetFeedCallback callback) {
        if (!controller->current_feed_.hash.empty()) {
          auto clone = controller->current_feed_.Clone();
          std::move(callback).Run(std::move(clone));
          return;
        } else {
          // There was a problem fetching the feed.
          std::move(callback).Run(brave_news::mojom::Feed::New());
        }
      },
      base::Unretained(this), std::move(callback)));
}

void FeedController::EnsureFeedIsUpdating() {
  VLOG(1) << "EnsureFeedIsUpdating " << is_update_in_progress_;
  // Only 1 update at a time, other calls for data will wait for
  // the current operation via the `on_publishers_update_` OneShotEvent.
  if (is_update_in_progress_) {
    return;
  }
  is_update_in_progress_ = true;

  // Fetch publishers via callback
  publishers_controller_->GetOrFetchPublishers(base::BindOnce(
      [](base::WeakPtr<FeedController> controller, Publishers publishers) {
        if (!controller) {
          return;
        }

        // Handle no publishers
        if (publishers.empty()) {
          LOG(ERROR) << "Brave News Publisher list was empty";
          controller->NotifyUpdateDone();
          return;
        }

        // Handle all feed items downloaded
        // Fetch https request via callback
        controller->feed_fetcher_.FetchFeed(base::BindOnce(
            [](base::WeakPtr<FeedController> controller, Publishers publishers,
               FeedItems items, ETags etags) {
              if (!controller) {
                return;
              }

              controller->locale_feed_etags_ = std::move(etags);

              VLOG(1) << "All feed item fetches done with item count: "
                      << items.size();
              if (items.size() == 0) {
                controller->ResetFeed();
                controller->NotifyUpdateDone();
                return;
              }

              // Get history hosts via callback
              auto on_history = base::BindOnce(
                  [](base::WeakPtr<FeedController> controller, FeedItems items,
                     Publishers publishers, history::QueryResults results) {
                    if (!controller) {
                      return;
                    }

                    std::unordered_set<std::string> history_hosts;
                    for (const auto& item : results) {
                      auto host = item.url().host();
                      history_hosts.insert(host);
                    }
                    VLOG(1) << "history hosts # " << history_hosts.size();
                    // Parse directly to in-memory property
                    controller->ResetFeed();
                    std::vector<mojom::FeedItemPtr> feed_items;
                    if (BuildFeed(items, history_hosts, &publishers,
                                  &controller->current_feed_,
                                  controller->prefs_)) {
                    } else {
                      VLOG(1) << "ParseFeed reported failure.";
                    }
                    // Let any callbacks know that the data is ready
                    // or errored.
                    controller->NotifyUpdateDone();
                  },
                  controller->weak_ptr_factory_.GetWeakPtr(), std::move(items),
                  std::move(publishers));
              history::QueryOptions options;
              options.max_count = 2000;
              options.SetRecentDayRange(14);
              controller->history_service_->QueryHistory(
                  std::u16string(), options, std::move(on_history),
                  &controller->task_tracker_);
            },
            controller->weak_ptr_factory_.GetWeakPtr(), std::move(publishers)));
      },
      weak_ptr_factory_.GetWeakPtr()));
}

void FeedController::EnsureFeedIsCached() {
  VLOG(1) << "EnsureFeedIsCached";
  GetOrFetchFeed(
      base::BindOnce([]() { VLOG(1) << "EnsureFeedIsCached callback"; }));
}

void FeedController::UpdateIfRemoteChanged() {
  // If already updating, nothing to do,
  // we don't want to collide with an update
  // which starts and completes before our HEAD
  // request completes (which admittedly is very unlikely).
  if (is_update_in_progress_) {
    return;
  }

  feed_fetcher_.IsUpdateAvailable(
      locale_feed_etags_,
      base::BindOnce(
          [](FeedController* controller, bool has_update) {
            if (!has_update) {
              return;
            }

            controller->EnsureFeedIsUpdating();
          },
          // Note: Unretained is safe here because this class owns the
          // FeedFetcher, which uses WeakPtrs internally.
          base::Unretained(this)));
}

void FeedController::ClearCache() {
  ResetFeed();
}

void FeedController::OnPublishersUpdated(PublishersController* controller) {
  VLOG(1) << "OnPublishersUpdated";
  EnsureFeedIsUpdating();
}

void FeedController::GetOrFetchFeed(base::OnceClosure callback) {
  VLOG(1) << "getorfetch feed(oc) start: "
          << on_current_update_complete_->is_signaled();
  // If in-memory feed is, no need to wait, otherwise wait for fetch
  // to be complete.
  if (!current_feed_.hash.empty()) {
    VLOG(1) << "getorfetchfeed(oc) from cache";
    std::move(callback).Run();
    return;
  }
  // Ensure feed is currently being fetched.
  // Subscribe to result of current feed fetch.
  on_current_update_complete_->Post(FROM_HERE, std::move(callback));
  EnsureFeedIsUpdating();
}

void FeedController::ResetFeed() {
  current_feed_.featured_item = nullptr;
  current_feed_.hash = "";
  current_feed_.pages.clear();
}

void FeedController::NotifyUpdateDone() {
  // Let any callbacks know that the data is ready.
  on_current_update_complete_->Signal();
  // Reset the OneShotEvent so that future requests
  // can be waited for.
  is_update_in_progress_ = false;
  on_current_update_complete_ = std::make_unique<base::OneShotEvent>();

  // Notify listeners.
  for (const auto& listener : listeners_) {
    listener->OnUpdateAvailable(current_feed_.hash);
  }
}

}  // namespace brave_news
