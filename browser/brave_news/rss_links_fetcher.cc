// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/brave_news/rss_links_fetcher.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace brave_news {

namespace {

mojo::Remote<mojom::RssLinkReader> GetRssLinkReaderRemote(
    content::WebContents* web_contents) {
  CHECK(web_contents->GetPrimaryMainFrame()->IsRenderFrameLive());
  mojo::Remote<mojom::RssLinkReader> result;
  // GetRemoteInterfaces() cannot be null if the render frame is created.
  web_contents->GetPrimaryMainFrame()->GetRemoteInterfaces()->GetInterface(
      result.BindNewPipeAndPassReceiver());
  return result;
}

class RssLinksFetcher {
 public:
  void Start(const GURL& page_url,
             mojo::Remote<mojom::RssLinkReader> link_reader,
             base::OnceCallback<void(std::vector<GURL>)> callback) {
    page_url_ = page_url;
    callback_ = std::move(callback);
    link_reader_ = std::move(link_reader);
    if (link_reader_) {
      // Unretained is OK here. The `mojo::Remote` will not invoke callbacks
      // after it is destroyed.
      link_reader_.set_disconnect_handler(base::BindOnce(
          &RssLinksFetcher::SendResultAndDeleteSelf, base::Unretained(this)));
      link_reader_->GetRssLinks(base::BindOnce(
          &RssLinksFetcher::GetRssLinksComplete, base::Unretained(this)));
      return;
    }
    SendResultAndDeleteSelf();
  }

 private:
  void GetRssLinksComplete(mojom::RssLinksPtr rss_links) {
    if (rss_links && rss_links->page_url == page_url_) {
      for (GURL& link : rss_links->links) {
        if (link.is_valid() && link.SchemeIsHTTPOrHTTPS()) {
          result_.push_back(std::move(link));
        }
      }
    }
    SendResultAndDeleteSelf();
  }

  void SendResultAndDeleteSelf() {
    std::move(callback_).Run(std::move(result_));
    delete this;
  }

  GURL page_url_;
  mojo::Remote<mojom::RssLinkReader> link_reader_;
  std::vector<GURL> result_;
  base::OnceCallback<void(std::vector<GURL>)> callback_;
};

void FetchRssLinksHelper(const GURL& url,
                         mojo::Remote<mojom::RssLinkReader> link_reader,
                         base::OnceCallback<void(std::vector<GURL>)> callback) {
  // RssLinksFetcher is self-deleting.
  auto* fetcher = new RssLinksFetcher();
  fetcher->Start(url, std::move(link_reader), std::move(callback));
}

}  // namespace

void FetchRssLinksForTesting(
    const GURL& url,
    mojo::Remote<mojom::RssLinkReader> link_reader,
    base::OnceCallback<void(std::vector<GURL>)> callback) {
  FetchRssLinksHelper(url, std::move(link_reader), std::move(callback));
}

void FetchRssLinks(const GURL& url,
                   content::WebContents* web_contents,
                   base::OnceCallback<void(std::vector<GURL>)> callback) {
  if (!web_contents) {
    std::move(callback).Run(std::vector<GURL>());
    return;
  }
  FetchRssLinksHelper(url, GetRssLinkReaderRemote(web_contents),
                      std::move(callback));
}

}  // namespace brave_news
