// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_controller.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "brave/components/brave_news/browser/background_history_querier.h"
#include "brave/components/brave_news/browser/brave_news_engine.h"
#include "brave/components/brave_news/browser/feed_building.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/feed_v2_builder.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

FeedController::FeedController(
    PublishersController* publishers_controller,
    BackgroundHistoryQuerier& history_querier,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    base::WeakPtr<DirectFeedFetcher::Delegate> direct_feed_fetcher_delegate)
    : publishers_controller_(publishers_controller),
      history_querier_(history_querier),
      feed_fetcher_(*publishers_controller,
                    url_loader_factory,
                    direct_feed_fetcher_delegate),
      on_current_update_complete_(new base::OneShotEvent()) {}

FeedController::~FeedController() = default;

void FeedController::GetOrFetchFeed(const SubscriptionsSnapshot& subscriptions,
                                    GetFeedCallback callback) {
  GetOrFetchFeed(
      subscriptions,
      base::BindOnce(
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

void FeedController::EnsureFeedIsUpdating(
    const SubscriptionsSnapshot& subscriptions) {
  VLOG(1) << "EnsureFeedIsUpdating " << is_update_in_progress_;
  // Only 1 update at a time, other calls for data will wait for
  // the current operation via the `on_publishers_update_` OneShotEvent.
  if (is_update_in_progress_) {
    return;
  }
  is_update_in_progress_ = true;

  // Fetch publishers via callback
  publishers_controller_->GetOrFetchPublishers(
      subscriptions,
      base::BindOnce(
          [](base::WeakPtr<FeedController> controller,
             const SubscriptionsSnapshot& subscriptions,
             Publishers publishers) {
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
            controller->feed_fetcher_.FetchFeed(
                subscriptions,
                base::BindOnce(
                    [](base::WeakPtr<FeedController> controller,
                       const SubscriptionsSnapshot& subscriptions,
                       Publishers publishers, FeedItems items, ETags etags) {
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
                          [](base::WeakPtr<FeedController> controller,
                             const SubscriptionsSnapshot& subscriptions,
                             FeedItems items, Publishers publishers,
                             history::QueryResults results) {
                            if (!controller) {
                              return;
                            }

                            std::unordered_set<std::string> history_hosts;
                            for (const auto& item : results) {
                              auto host = item.url().host();
                              history_hosts.insert(host);
                            }
                            VLOG(1)
                                << "history hosts # " << history_hosts.size();
                            // Parse directly to in-memory property
                            controller->ResetFeed();

                            // Store the subscriptions we used to generate this
                            // feed.
                            controller->last_subscriptions_ = subscriptions;
                            std::vector<mojom::FeedItemPtr> feed_items;
                            if (!BuildFeed(items, history_hosts, &publishers,
                                           &controller->current_feed_,
                                           subscriptions)) {
                              VLOG(1) << "ParseFeed reported failure.";
                            }
                            // Let any callbacks know that the data is ready
                            // or errored.
                            controller->NotifyUpdateDone();
                          },
                          controller->weak_ptr_factory_.GetWeakPtr(),
                          subscriptions, std::move(items),
                          std::move(publishers));
                      controller->history_querier_->Run(std::move(on_history));
                    },
                    controller->weak_ptr_factory_.GetWeakPtr(), subscriptions,
                    std::move(publishers)));
          },
          weak_ptr_factory_.GetWeakPtr(), subscriptions));
}

void FeedController::EnsureFeedIsCached(
    const SubscriptionsSnapshot& subscriptions) {
  VLOG(1) << "EnsureFeedIsCached";
  GetOrFetchFeed(subscriptions, base::BindOnce([]() {
                   VLOG(1) << "EnsureFeedIsCached callback";
                 }));
}

void FeedController::UpdateIfRemoteChanged(
    const SubscriptionsSnapshot& subscriptions,
    HashCallback callback) {
  auto hash_callback = base::BindOnce(
      [](base::WeakPtr<FeedController> controller, HashCallback callback) {
        if (!controller) {
          return;
        }

        std::move(callback).Run(controller->current_feed_.hash);
      },
      weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  // If already updating, update with the hash once the update is complete.
  // We don't want to collide with an update which starts and completes before
  // our HEAD request completes (which admittedly is very unlikely).
  if (is_update_in_progress_) {
    on_current_update_complete_->Post(FROM_HERE, std::move(hash_callback));
    return;
  }

  // If the subscriptions have changed, we don't need to check the remote to
  // know we need to update the feed.
  if (!subscriptions.DiffPublishers(last_subscriptions_).IsEmpty() ||
      !subscriptions.DiffChannels(last_subscriptions_).IsEmpty()) {
    EnsureFeedIsUpdating(subscriptions);
    on_current_update_complete_->Post(FROM_HERE, std::move(hash_callback));
    return;
  }

  feed_fetcher_.IsUpdateAvailable(
      subscriptions, locale_feed_etags_,
      base::BindOnce(
          [](FeedController* controller,
             const SubscriptionsSnapshot& subscriptions,
             base::OnceClosure callback, bool has_update) {
            if (!has_update) {
              std::move(callback).Run();
              return;
            }

            // If the remote feeds have changed, refetch/regenerate the feed and
            // fire the callback with the new hash.
            controller->EnsureFeedIsUpdating(subscriptions);
            controller->on_current_update_complete_->Post(FROM_HERE,
                                                          std::move(callback));
          },
          // Note: Unretained is safe here because this class owns the
          // FeedFetcher, which uses WeakPtrs internally.
          base::Unretained(this), subscriptions, std::move(hash_callback)));
}

void FeedController::ClearCache() {
  ResetFeed();
}

void FeedController::GetOrFetchFeed(const SubscriptionsSnapshot& subscriptions,
                                    base::OnceClosure callback) {
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
  EnsureFeedIsUpdating(subscriptions);
}

void FeedController::ResetFeed() {
  current_feed_.featured_item = nullptr;
  current_feed_.hash = "";
  current_feed_.pages.clear();
  last_subscriptions_ = {};
}

void FeedController::NotifyUpdateDone() {
  // Let any callbacks know that the data is ready.
  on_current_update_complete_->Signal();
  // Reset the OneShotEvent so that future requests
  // can be waited for.
  is_update_in_progress_ = false;
  on_current_update_complete_ = std::make_unique<base::OneShotEvent>();
}

}  // namespace brave_news
