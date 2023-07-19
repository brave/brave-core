// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_CONTROLLER_H_

#include <list>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_news/browser/direct_feed_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom-shared.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "brave/components/brave_news/rust/lib.rs.h"
#include "url/gurl.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_news {

constexpr std::size_t kMaxArticlesPerDirectFeedSource = 100;

struct DirectFeedResponse {
 public:
  FeedData data;
  GURL url;
  bool success = false;
};

using Articles = std::vector<mojom::ArticlePtr>;
using GetArticlesCallback = base::OnceCallback<void(Articles)>;
using GetFeedItemsCallback =
    base::OnceCallback<void(std::vector<mojom::FeedItemPtr>)>;
using IsValidCallback =
    base::OnceCallback<void(const bool is_valid, const std::string& title)>;

// Controls RSS / Atom / JSON / etc. feeds - those downloaded
// directly from the feed source server.
class DirectFeedController {
 public:
  explicit DirectFeedController(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~DirectFeedController();
  DirectFeedController(const DirectFeedController&) = delete;
  DirectFeedController& operator=(const DirectFeedController&) = delete;

  // Adds a direct feed pref. Returns false if the publisher already exists, and
  // true otherwise.
  bool AddDirectFeedPref(const GURL& feed_url,
                         const std::string& title,
                         const absl::optional<std::string>& id = absl::nullopt);

  // Removes a direct feed pref
  void RemoveDirectFeedPref(const std::string& publisher_id);

  // Returns a list of all the direct feeds currently subscribed to.
  std::vector<mojom::PublisherPtr> ParseDirectFeedsPref();
  void VerifyFeedUrl(const GURL& feed_url, IsValidCallback callback);
  void DownloadAllContent(std::vector<mojom::PublisherPtr> publishers,
                          GetFeedItemsCallback callback);
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

  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  FRIEND_TEST_ALL_PREFIXES(BraveNewsDirectFeed, ParseToArticle);
  FRIEND_TEST_ALL_PREFIXES(BraveNewsDirectFeed, ParseOnlyAllowsHTTPLinks);

  // Convert from a parsed feed's FeedData to the mojom Article objects used
  // in Brave News.
  static void BuildArticles(Articles& articles,
                            const FeedData& data,
                            const std::string& publisher_id);
  void DownloadFeedContent(const GURL& feed_url,
                           const std::string& publisher_id,
                           GetArticlesCallback callback);
  void DownloadFeed(const GURL& feed_url, DownloadFeedCallback callback);
  void OnResponse(SimpleURLLoaderList::iterator iter,
                  DownloadFeedCallback callback,
                  const GURL& feed_url,
                  const std::unique_ptr<std::string> response_body);

  void FindFeedsImpl(const GURL& possible_feed_or_site_url);
  void OnFindFeedsImplResponse(
      const GURL& feed_url,
      std::vector<mojom::FeedSearchResultItemPtr> results);

  raw_ptr<PrefService> prefs_;
  SimpleURLLoaderList url_loaders_;
  DirectFeedFetcher fetcher_;

  // TODO(sko) We should have a way to cancel requests.
  // e.g. Navigate to different sites, quit app.
  // Witthout that, some heavy RSS feed parsing work will prevent new feeds from
  // detection and app from shutdown.
  std::queue<FindFeedRequest> pending_requests_;
  base::flat_map<GURL, std::vector<FindFeedRequest>> ongoing_requests_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  base::WeakPtrFactory<DirectFeedController> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_CONTROLLER_H_
