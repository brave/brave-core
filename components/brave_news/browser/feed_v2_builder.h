// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_BUILDER_H_

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

using BuildFeedCallback = mojom::BraveNewsController::GetFeedV2Callback;

class FeedV2Builder {
 public:
  FeedV2Builder(
      PublishersController& publishers_controller,
      ChannelsController& channels_controller,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  FeedV2Builder(const FeedV2Builder&) = delete;
  FeedV2Builder& operator=(const FeedV2Builder&) = delete;
  ~FeedV2Builder();

  void Build(BuildFeedCallback callback);

 private:
  void BuildFeedFromArticles(BuildFeedCallback callback);

  FeedFetcher fetcher_;

  FeedItems raw_feed_items_;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_V2_BUILDER_H_
