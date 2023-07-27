// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_FETCHER_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace brave_news {

constexpr std::size_t kMaxArticlesPerDirectFeedSource = 100;

struct FeedData;

struct DirectFeedError {
  std::string body_content;
};

struct DirectFeedResult {
  std::string id;
  std::string title;
  std::vector<mojom::ArticlePtr> articles;

  DirectFeedResult();
  ~DirectFeedResult();
  DirectFeedResult(DirectFeedResult&&);
  DirectFeedResult& operator=(DirectFeedResult&&);
  DirectFeedResult(const DirectFeedResult&) = delete;
  DirectFeedResult& operator=(const DirectFeedResult&) = delete;
};

struct DirectFeedResponse {
  GURL url;
  GURL final_url;
  std::string mime_type;
  std::string charset;

  // If success, this will be |FeedData|, otherwise an error.
  absl::variant<DirectFeedResult, DirectFeedError> result;

  DirectFeedResponse();
  ~DirectFeedResponse();
  DirectFeedResponse(DirectFeedResponse&&);
  DirectFeedResponse(const DirectFeedResponse&) = delete;
  DirectFeedResponse& operator=(const DirectFeedResponse&) = delete;
};

using DownloadFeedCallback = base::OnceCallback<void(DirectFeedResponse)>;

// Exposed for testing.
void ConvertFeedDataToArticles(std::vector<mojom::ArticlePtr>& articles,
                               FeedData data,
                               const std::string& publisher_id);

class DirectFeedFetcher {
 public:
  explicit DirectFeedFetcher(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  DirectFeedFetcher(const DirectFeedFetcher&) = delete;
  DirectFeedFetcher& operator=(const DirectFeedFetcher&) = delete;
  ~DirectFeedFetcher();

  // |publisher_id| can be empty, if one we're speculatively downloading a feed.
  // This |publisher_id| will be used for any returned articles.
  void DownloadFeed(const GURL& url,
                    std::string publisher_id,
                    DownloadFeedCallback callback);

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  void OnFeedDownloaded(SimpleURLLoaderList::iterator iter,
                        DownloadFeedCallback callback,
                        const GURL& feed_url,
                        std::string publisher_id,
                        const std::unique_ptr<std::string> response_body);
  void OnParsedFeedData(DownloadFeedCallback callback,
                        DirectFeedResponse result,
                        absl::variant<DirectFeedResult, DirectFeedError> data);

  SimpleURLLoaderList url_loaders_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<DirectFeedFetcher> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_FETCHER_H_
