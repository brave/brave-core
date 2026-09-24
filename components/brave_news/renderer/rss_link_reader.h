// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_RENDERER_RSS_LINK_READER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_RENDERER_RSS_LINK_READER_H_

#include "brave/components/brave_news/common/rss_link_reader.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {
class RenderFrame;
}

namespace brave_news {

// Implements `mojom::RssLinkReader`. Lifted from upstream's
// `feed::RssLinkReader` (deleted in cr/7795938 along with the Web Feed
// feature) so Brave News can keep discovering RSS feeds on visited pages.
class RssLinkReader : public content::RenderFrameObserver,
                      public mojom::RssLinkReader {
 public:
  RssLinkReader(content::RenderFrame* render_frame,
                service_manager::BinderRegistry* registry);

  RssLinkReader(const RssLinkReader&) = delete;
  RssLinkReader& operator=(const RssLinkReader&) = delete;

  ~RssLinkReader() override;

  // mojom::RssLinkReader:
  void GetRssLinks(GetRssLinksCallback callback) override;

  // content::RenderFrameObserver:
  void OnDestruct() override;

 private:
  void BindReceiver(mojo::PendingReceiver<mojom::RssLinkReader> receiver);

  mojo::Receiver<mojom::RssLinkReader> receiver_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_RENDERER_RSS_LINK_READER_H_
