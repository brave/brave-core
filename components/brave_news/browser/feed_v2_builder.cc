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
  if (articles_.size()) {
    BuildFeedFromArticles(std::move(callback));
    return;
  }

  fetcher_.FetchFeed(base::BindOnce(
      [](FeedV2Builder* builder, BuildFeedCallback callback, FeedItems items,
         ETags tags) {
        std::vector<mojom::ArticlePtr> articles;
        for (const auto& item : items) {
          if (item->is_article()) {
            articles.push_back(std::move(item->get_article()));
          }
        }
        builder->articles_ = std::move(articles);
        builder->BuildFeedFromArticles(std::move(callback));
      },
      // Unretained is safe here because the FeedFetcher is owned by this and
      // uses weak pointers internally.
      base::Unretained(this), std::move(callback)));
}

void FeedV2Builder::BuildFeedFromArticles(BuildFeedCallback callback) {
  std::vector<mojom::ArticlePtr> articles;
  base::ranges::transform(articles_, std::back_inserter(articles),
                          [](const auto& article) { return article->Clone(); });
  std::move(callback).Run(std::move(articles));
}

}  // namespace brave_news
