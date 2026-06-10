// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_BRAVE_NEWS_RSS_LINKS_FETCHER_H_
#define BRAVE_BROWSER_BRAVE_NEWS_RSS_LINKS_FETCHER_H_

#include <vector>

#include "base/functional/callback_forward.h"
#include "brave/components/brave_news/common/rss_link_reader.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

namespace brave_news {

// Fetches all RSS links associated with the main frame of `web_contents`.
// Lifted from upstream's `feed::FetchRssLinks`, deleted in cr/7795938.
void FetchRssLinks(const GURL& url,
                   content::WebContents* web_contents,
                   base::OnceCallback<void(std::vector<GURL>)> callback);

void FetchRssLinksForTesting(
    const GURL& url,
    mojo::Remote<mojom::RssLinkReader> link_reader,
    base::OnceCallback<void(std::vector<GURL>)> callback);

}  // namespace brave_news

#endif  // BRAVE_BROWSER_BRAVE_NEWS_RSS_LINKS_FETCHER_H_
