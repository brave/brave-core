// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/one_shot_event.h"
#include "base/scoped_observation.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_today/browser/direct_feed_controller.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "components/history/core/browser/history_service.h"

namespace history {
class HistoryService;
}  // namespace history

namespace brave_news {

using GetFeedCallback = mojom::BraveNewsController::GetFeedCallback;
using FeedItems = std::vector<mojom::FeedItemPtr>;
using GetFeedItemsCallback = base::OnceCallback<void(FeedItems)>;

class FeedController : public PublishersController::Observer {
 public:
  FeedController(PublishersController* publishers_controller,
                 DirectFeedController* direct_feed_controller,
                 history::HistoryService* history_service,
                 api_request_helper::APIRequestHelper* api_request_helper);
  ~FeedController() override;
  FeedController(const FeedController&) = delete;
  FeedController& operator=(const FeedController&) = delete;

  // Checks if latest cached (or in-progress fetched) feed matches incoming hash
  void DoesFeedVersionDiffer(
      const std::string& matching_hash,
      mojom::BraveNewsController::IsFeedUpdateAvailableCallback callback);
  // Provides a clone of data so that caller can take ownership or dispose
  void GetOrFetchFeed(GetFeedCallback callback);
  // Perform an update to the feed from source, but not more than once
  // if a fetch is already in-progress.
  void EnsureFeedIsUpdating();
  // Same as GetOrFetchFeed with no callback - ensures that a fetch has
  // occured and that we have data (if there was no problem fetching or
  // parsing).
  void EnsureFeedIsCached();
  void UpdateIfRemoteChanged();
  void ClearCache();

  // PublishersController::Observer
  //
  // We need to know when Publishers changes so that we can fetch
  // or at least re-parse the feed and either exclude or include
  // new, removed or turned-off publishers (according to either user-preference
  // or remote defaults).
  void OnPublishersUpdated(PublishersController* publishers) override;

 private:
  void FetchCombinedFeed(GetFeedItemsCallback callback);
  void GetOrFetchFeed(base::OnceClosure callback);
  void ResetFeed();
  void NotifyUpdateDone();

  raw_ptr<PublishersController> publishers_controller_ = nullptr;
  raw_ptr<DirectFeedController> direct_feed_controller_ = nullptr;
  raw_ptr<history::HistoryService> history_service_ = nullptr;
  raw_ptr<api_request_helper::APIRequestHelper> api_request_helper_ = nullptr;

  // The task tracker for the HistoryService callbacks.
  base::CancelableTaskTracker task_tracker_;
  // Internal callers subscribe to this to know when the current in-progress
  // fetch and parse is complete.
  std::unique_ptr<base::OneShotEvent> on_current_update_complete_;
  base::ScopedObservation<PublishersController, PublishersController::Observer>
      publishers_observation_;
  // Store a copy of the feed in memory so we don't fetch new data from remote
  // every time the UI opens.
  mojom::Feed current_feed_;
  std::string current_feed_etag_;
  bool is_update_in_progress_ = false;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_CONTROLLER_H_
