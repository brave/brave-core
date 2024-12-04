// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_FETCHER_H_

#include <string>
#include <tuple>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/direct_feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"

namespace brave_news {

using FeedItems = std::vector<mojom::FeedItemPtr>;
using ETags = base::flat_map<std::string, std::string>;
using FetchFeedCallback = base::OnceCallback<void(FeedItems items, ETags tags)>;
using UpdateAvailableCallback = base::OnceCallback<void(bool)>;

class SubscriptionsSnapshot;

class FeedFetcher {
 public:
  FeedFetcher(PublishersController& publishers_controller,
              scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
              DirectFeedFetcher::Delegate* delegate);
  ~FeedFetcher();
  FeedFetcher(const FeedFetcher&) = delete;
  FeedFetcher& operator=(const FeedFetcher&) = delete;

  void FetchFeed(const SubscriptionsSnapshot& subscriptions,
                 FetchFeedCallback callback);

  void IsUpdateAvailable(const SubscriptionsSnapshot& subscriptions,
                         ETags etags,
                         UpdateAvailableCallback callback);

 private:
  struct FeedSourceResult {
    std::string key;
    std::string etag;
    FeedItems items;

    FeedSourceResult();
    FeedSourceResult(std::string key, std::string etag, FeedItems items);
    ~FeedSourceResult();
    FeedSourceResult(FeedSourceResult&&);
    FeedSourceResult(const FeedSourceResult&) = delete;
    FeedSourceResult& operator=(const FeedSourceResult&) = delete;
  };
  using FetchFeedSourceCallback =
      base::OnceCallback<void(FeedSourceResult items)>;
  static std::tuple<FeedItems, ETags> CombineFeedSourceResults(
      std::vector<FeedSourceResult> results);

  // Steps for |FetchFeed|
  void OnFetchFeedFetchedPublishers(const SubscriptionsSnapshot& subscriptions,
                                    FetchFeedCallback callback,
                                    Publishers publishers);
  void OnFetchFeedFetchedFeed(std::string locale,
                              FetchFeedSourceCallback callback,
                              api_request_helper::APIRequestResult result);
  void OnFetchFeedFetchedAll(FetchFeedCallback callback,
                             Publishers publishers,
                             std::vector<FeedSourceResult> results);

  // Steps for |IsUpdateAvailable|
  void OnIsUpdateAvailableFetchedPublishers(
      const SubscriptionsSnapshot& subscriptions,
      ETags etags,
      UpdateAvailableCallback callback,
      Publishers publishers);
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
