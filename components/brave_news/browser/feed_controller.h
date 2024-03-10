// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "components/history/core/browser/history_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace history {
class HistoryService;
}  // namespace history

namespace brave_news {

using GetFeedCallback = mojom::BraveNewsController::GetFeedCallback;
using FeedItems = std::vector<mojom::FeedItemPtr>;
using GetFeedItemsCallback = base::OnceCallback<void(FeedItems)>;

class FeedController {
 public:
  FeedController(
      PublishersController* publishers_controller,
      history::HistoryService* history_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~FeedController();
  FeedController(const FeedController&) = delete;
  FeedController& operator=(const FeedController&) = delete;

  // Checks if latest cached (or in-progress fetched) feed matches incoming hash
  void DoesFeedVersionDiffer(
      const BraveNewsSubscriptions& subscriptions,
      const std::string& matching_hash,
      mojom::BraveNewsController::IsFeedUpdateAvailableCallback callback);
  // Adds a listener which will be notified of feed updates.
  void AddListener(mojo::PendingRemote<mojom::FeedListener> listener);
  // Provides a clone of data so that caller can take ownership or dispose
  void GetOrFetchFeed(const BraveNewsSubscriptions& subscriptions,
                      GetFeedCallback callback);
  // Perform an update to the feed from source, but not more than once
  // if a fetch is already in-progress.
  void EnsureFeedIsUpdating(const BraveNewsSubscriptions& subscriptions);
  // Same as GetOrFetchFeed with no callback - ensures that a fetch has
  // occured and that we have data (if there was no problem fetching or
  // parsing).
  void EnsureFeedIsCached(const BraveNewsSubscriptions& subscriptions);
  void UpdateIfRemoteChanged(const BraveNewsSubscriptions& subscriptions);
  void ClearCache();

 private:
  void GetOrFetchFeed(const BraveNewsSubscriptions& subscriptions,
                      base::OnceClosure callback);
  void ResetFeed();
  void NotifyUpdateDone();

  raw_ptr<PublishersController> publishers_controller_ = nullptr;
  raw_ptr<history::HistoryService> history_service_ = nullptr;

  FeedFetcher feed_fetcher_;

  // The task tracker for the HistoryService callbacks.
  base::CancelableTaskTracker task_tracker_;
  // Internal callers subscribe to this to know when the current in-progress
  // fetch and parse is complete.
  std::unique_ptr<base::OneShotEvent> on_current_update_complete_;
  mojo::RemoteSet<mojom::FeedListener> listeners_;
  // Store a copy of the feed in memory so we don't fetch new data from remote
  // every time the UI opens.
  mojom::Feed current_feed_;

  // A map from feed locale to the last known etag for that feed. Used to
  // determine when we have available updates.
  base::flat_map<std::string, std::string> locale_feed_etags_;
  bool is_update_in_progress_ = false;

  base::WeakPtrFactory<FeedController> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_CONTROLLER_H_
