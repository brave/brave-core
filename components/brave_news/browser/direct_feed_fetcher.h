// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_FETCHER_H_

#include <list>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_news/rust/lib.rs.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "url/gurl.h"

namespace brave_news {

struct DirectFeedData {
  FeedData data;
  GURL url;
  bool success = false;
};

using DownloadFeedCallback = base::OnceCallback<void(DirectFeedData)>;

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
                        DirectFeedData result,
                        absl::optional<FeedData> data);

  SimpleURLLoaderList url_loaders_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<DirectFeedFetcher> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_FETCHER_H_
