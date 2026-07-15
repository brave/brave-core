// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_FETCHER_H_

#include <string>
#include <tuple>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/direct_feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace brave_news {

using FeedItems = std::vector<mojom::FeedItemPtr>;
using ETags = absl::flat_hash_map<std::string, std::string>;
// |connection_error| is true only when we failed to reach Brave's own feed
// server (i.e. the user appears to be offline). A failing direct (custom RSS)
// feed does not set this, since a single unreachable third-party feed does not
// mean the whole feed failed to load.
using FetchFeedCallback = base::OnceCallback<
    void(FeedItems items, ETags tags, bool connection_error)>;
using UpdateAvailableCallback = base::OnceCallback<void(bool)>;

class SubscriptionsSnapshot;

class FeedFetcher {
 public:
  FeedFetcher(PublishersController& publishers_controller,
              scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
              base::WeakPtr<DirectFeedFetcher::Delegate> delegate);
  ~FeedFetcher();
  FeedFetcher(const FeedFetcher&) = delete;
  FeedFetcher& operator=(const FeedFetcher&) = delete;

  void FetchFeed(const SubscriptionsSnapshot& subscriptions,
                 FetchFeedCallback callback);

  void IsUpdateAvailable(const SubscriptionsSnapshot& subscriptions,
                         ETags etags,
                         UpdateAvailableCallback callback);

 private:
  friend class FeedFetcherTest;

  struct FeedSourceResult {
    std::string key;
    std::string etag;
    FeedItems items;
    // Whether this source produced a usable response. A source can succeed but
    // still contribute no items (e.g. a valid but empty feed).
    bool success = false;
    // Whether fetching this source failed with a connection/network error.
    // Only set for Brave's own feed sources: a failing direct (custom RSS) feed
    // does not imply the user is offline, so it never sets this.
    bool connection_error = false;

    FeedSourceResult();
    ~FeedSourceResult();
    FeedSourceResult(FeedSourceResult&&);
    FeedSourceResult(const FeedSourceResult&) = delete;
    FeedSourceResult& operator=(const FeedSourceResult&) = delete;
  };
  using FetchFeedSourceCallback =
      base::OnceCallback<void(FeedSourceResult items)>;
  // Returns the combined feed items, their etags, and whether a connection
  // error occurred while reaching Brave's feed server.
  static std::tuple<FeedItems, ETags, bool> CombineFeedSourceResults(
      std::vector<FeedSourceResult> results);

  // Steps for |FetchFeed|
  void OnFetchFeedFetchedPublishers(const SubscriptionsSnapshot& subscriptions,
                                    FetchFeedCallback callback,
                                    const Publishers& publishers);
  void OnFetchFeedFetchedFeed(std::string locale,
                              FetchFeedSourceCallback callback,
                              api_request_helper::APIRequestResult result);
  void OnFetchFeedFetchedAll(FetchFeedCallback callback,
                             std::vector<FeedSourceResult> results);

  // Steps for |IsUpdateAvailable|
  void OnIsUpdateAvailableFetchedPublishers(
      const SubscriptionsSnapshot& subscriptions,
      ETags etags,
      UpdateAvailableCallback callback,
      const Publishers& publishers);
  void OnIsUpdateAvailableFetchedHead(
      std::string current_etag,
      base::RepeatingCallback<void(bool)> has_update_callback,
      api_request_helper::APIRequestResult result);
  void OnIsUpdateAvailableCheckedFeeds(UpdateAvailableCallback callback,
                                       std::vector<bool> updates);

  const raw_ref<PublishersController> publishers_controller_;

  api_request_helper::APIRequestHelper api_request_helper_;
  DirectFeedFetcher direct_feed_fetcher_;

  base::WeakPtrFactory<FeedFetcher> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_FETCHER_H_
