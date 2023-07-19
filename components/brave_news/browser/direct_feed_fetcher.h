// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_FETCHER_H_

#include <list>
#include <variant>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/rust/lib.rs.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace brave_news {

struct DirectFeedError {
  std::string body_content;
};

struct DirectFeedResult {
  std::string id;
  std::string title;
  std::vector<mojom::ArticlePtr> articles;
};

struct DirectFeedResponse {
  GURL url;
  GURL final_url;
  std::string mime_type;
  std::string charset;

  // If success, this will be |FeedData|, otherwise an error.
  absl::variant<DirectFeedResult, DirectFeedError> result;
};

using DownloadFeedCallback = base::OnceCallback<void(DirectFeedResponse)>;

class DirectFeedFetcher {
 public:
  explicit DirectFeedFetcher(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  DirectFeedFetcher(const DirectFeedFetcher&) = delete;
  DirectFeedFetcher& operator=(const DirectFeedFetcher&) = delete;
  ~DirectFeedFetcher();

  void DownloadFeed(const GURL& url, DownloadFeedCallback callback);

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  void OnFeedDownloaded(SimpleURLLoaderList::iterator iter,
                        DownloadFeedCallback callback,
                        const GURL& feed_url,
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
