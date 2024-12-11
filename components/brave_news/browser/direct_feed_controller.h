// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_CONTROLLER_H_

#include <queue>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_news/browser/direct_feed_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "url/gurl.h"

class PrefService;

namespace brave_news {

using IsValidCallback =
    base::OnceCallback<void(const bool is_valid, const std::string& title)>;

// Controls RSS / Atom / JSON / etc. feeds - those downloaded
// directly from the feed source server.
class DirectFeedController {
 public:
  explicit DirectFeedController(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      base::WeakPtr<DirectFeedFetcher::Delegate> direct_feed_fetcher_delegate);
  ~DirectFeedController();
  DirectFeedController(const DirectFeedController&) = delete;
  DirectFeedController& operator=(const DirectFeedController&) = delete;

  void VerifyFeedUrl(const GURL& feed_url, IsValidCallback callback);
  void FindFeeds(const GURL& possible_feed_or_site_url,
                 mojom::BraveNewsController::FindFeedsCallback callback);

 private:
  struct FindFeedRequest {
    FindFeedRequest(const GURL& possible_feed_or_site_url,
                    mojom::BraveNewsController::FindFeedsCallback callback);
    FindFeedRequest(FindFeedRequest&&);
    FindFeedRequest& operator=(FindFeedRequest&&);
    ~FindFeedRequest();

    GURL possible_feed_or_site_url;
    mojom::BraveNewsController::FindFeedsCallback callback;
  };

  // TODO(sko) We might want to adjust this value.
  static constexpr size_t kMaxOngoingRequests = 2;

  FRIEND_TEST_ALL_PREFIXES(BraveNewsDirectFeed, ParseToArticle);
  FRIEND_TEST_ALL_PREFIXES(BraveNewsDirectFeed, ParseOnlyAllowsHTTPLinks);

  void FindFeedsImpl(const GURL& possible_feed_or_site_url);
  void OnFindFeedsImplDownloadedFeed(const GURL& feed_url,
                                     DirectFeedResponse result);
  void OnFindFeedsImplResponse(
      const GURL& feed_url,
      std::vector<mojom::FeedSearchResultItemPtr> results);

  raw_ptr<PrefService> prefs_;
  DirectFeedFetcher fetcher_;

  // TODO(sko) We should have a way to cancel requests.
  // e.g. Navigate to different sites, quit app.
  // Witthout that, some heavy RSS feed parsing work will prevent new feeds from
  // detection and app from shutdown.
  std::queue<FindFeedRequest> pending_requests_;
  base::flat_map<GURL, std::vector<FindFeedRequest>> ongoing_requests_;

  base::WeakPtrFactory<DirectFeedController> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_CONTROLLER_H_
