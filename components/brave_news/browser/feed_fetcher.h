// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_FETCHER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/one_shot_event.h"
#include "base/scoped_observation.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"

namespace brave_news {

using FeedItems = std::vector<mojom::FeedItemPtr>;
using ETags = std::vector<std::string>;
using FetchFeedCallback = base::OnceCallback<void(FeedItems items, ETags tags)>;
using UpdateAvailableCallback = base::OnceCallback<void(bool)>;

class FeedFetcher {
 public:
  explicit FeedFetcher(
      PublishersController& publishers_controller,
      ChannelsController& channels_controller,
      api_request_helper::APIRequestHelper& api_request_helper);
  ~FeedFetcher();
  FeedFetcher(const FeedFetcher&) = delete;
  FeedFetcher& operator=(const FeedFetcher&) = delete;

  void FetchFeed(FetchFeedCallback callback);

  void IsUpdateAvailable(ETags etags, UpdateAvailableCallback callback);

 private:
  void OnIsUpdateAvailableFetchedPublishers(UpdateAvailableCallback callback,
                                            const Publishers& publishers);
  void OnIsUpdateAvailableFetchedHead(
      std::string current_etag,
      base::RepeatingCallback<void(bool)> has_update_callback,
      api_request_helper::APIRequestResult result);
  void OnIsUpdateAvailableCheckedFeeds(UpdateAvailableCallback callback,
                                       std::vector<bool> updates);

  const raw_ref<PublishersController> publishers_controller_;
  const raw_ref<ChannelsController> channels_controller_;
  const raw_ref<api_request_helper::APIRequestHelper> api_request_helper_;

  base::WeakPtrFactory<FeedFetcher> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_FETCHER_H_
