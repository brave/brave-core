// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/direct_feed_fetcher.h"

namespace brave_news {

DirectFeedFetcher::DirectFeedFetcher(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {}
DirectFeedFetcher::~DirectFeedFetcher() = default;

void DirectFeedFetcher::DownloadFeed(const GURL& url,
                                     DownloadFeedCallback callback) {}

}  // namespace brave_news
