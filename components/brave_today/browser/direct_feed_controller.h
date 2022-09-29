// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_DIRECT_FEED_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_DIRECT_FEED_CONTROLLER_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "brave/components/brave_today/rust/lib.rs.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
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
using DownloadFeedCallback =
    base::OnceCallback<void(std::unique_ptr<DirectFeedResponse>)>;
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
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  void DownloadFeedContent(const GURL& feed_url,
                           const std::string& publisher_id,
                           GetArticlesCallback callback);
  void DownloadFeed(const GURL& feed_url, DownloadFeedCallback callback);
  void OnResponse(SimpleURLLoaderList::iterator iter,
                  DownloadFeedCallback callback,
                  const GURL& feed_url,
                  const std::unique_ptr<std::string> response_body);

  raw_ptr<PrefService> prefs_;
  SimpleURLLoaderList url_loaders_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_DIRECT_FEED_CONTROLLER_H_
