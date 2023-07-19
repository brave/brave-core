// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_FETCHER_H_

#include "base/functional/callback_forward.h"
#include "brave/components/brave_news/rust/lib.rs.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
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
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_DIRECT_FEED_FETCHER_H_
