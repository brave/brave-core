// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_v2_builder.h"

#include <iterator>
#include <locale>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

FeedV2Builder::FeedV2Builder(
    PublishersController& publishers_controller,
    ChannelsController& channels_controller,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : fetcher_(publishers_controller, channels_controller, url_loader_factory) {

}

FeedV2Builder::~FeedV2Builder() = default;

void FeedV2Builder::Build(BuildFeedCallback callback) {
  if (raw_feed_items_.size()) {
    BuildFeedFromArticles(std::move(callback));
    return;
  }

  fetcher_.FetchFeed(base::BindOnce(
      [](FeedV2Builder* builder, BuildFeedCallback callback, FeedItems items,
         ETags tags) {
        builder->raw_feed_items_ = std::move(items);
        builder->BuildFeedFromArticles(std::move(callback));
      },
      // Unretained is safe here because the FeedFetcher is owned by this and
      // uses weak pointers internally.
      base::Unretained(this), std::move(callback)));
}

void FeedV2Builder::BuildFeedFromArticles(BuildFeedCallback callback) {
  auto feed = mojom::FeedV2::New();

  // TODO(fallaciousreasoning): Actually build the feed, rather than just adding
  // everything.
  for (const auto& item : raw_feed_items_) {
    if (item.is_null()) {
      continue;
    }
    if (item->is_article()) {
      feed->items.push_back(
          mojom::FeedItemV2::NewArticle(item->get_article()->Clone()));
    }
    if (item->is_promoted_article()) {
      feed->items.push_back(
          mojom::FeedItemV2::NewAdvert(item->get_promoted_article()->Clone()));
    }
  }
  std::move(callback).Run(std::move(feed));
}

}  // namespace brave_news
