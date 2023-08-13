// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_v2_builder.h"

#include <cstddef>
#include <iterator>
#include <locale>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/rand_util.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

namespace {

// constexpr int kBlockInlineMin = 1;
// constexpr int kBlockInlineMax = 5;
// constexpr double kInlineDiscoveryRatio = 0.25;
// constexpr int kSpecialCardEveryN = 2;
// constexpr double kAdsToDiscoverRatio = 0.25;
constexpr double kSourceSubscribedMin = 1e-6;
constexpr double kSourceSubscribedMax = 1;
constexpr double kSourceVisitsMin = 0.2;
constexpr double kPopRecencyHalfLifeInHours = 18;

// bool TossCoin() {
//   return base::RandDouble() < 0.5;
// }

double GetPopRecency(const mojom::ArticlePtr& article) {
  auto& publish_time = article->data->publish_time;

  // TODO(fallaciousreasoning): Use the new popularity field instead.
  double popularity = article->data->score == 0 ? 50 : article->data->score;
  double multiplier = publish_time > base::Time::Now() - base::Hours(5) ? 2 : 1;
  auto dt = base::Time::Now() - publish_time;

  return multiplier * popularity *
         pow(0.5, dt.InHours() / kPopRecencyHalfLifeInHours);
}

double GetArticleWeight(const mojom::ArticlePtr& article,
                        const Signals& signals) {
  auto it = signals.find(article->data->url.spec());
  if (it == signals.end()) {
    return 0.0;
  }

  const Signal& signal = it->second;
  double source_visits_projected =
      kSourceVisitsMin + signal.visit_weight * (1 - kSourceVisitsMin);
  double source_subscribed_projected =
      signal.subscribed ? kSourceSubscribedMax : kSourceSubscribedMin;
  return source_visits_projected * source_subscribed_projected *
         GetPopRecency(article);
}

mojom::ArticlePtr PickRouletteAndRemove(
    std::vector<mojom::ArticlePtr>& articles,
    const Signals& signals) {

  double total_weight = 0;
  for (const auto& article : articles) {
    total_weight += GetArticleWeight(article, signals);
  }

  // Non of the items are eligible to be picked.
  if (total_weight == 0) {
    return nullptr;
  }

  DCHECK_GT(articles.size(), 0u);

  double picked_value = base::RandDouble() * total_weight;
  double current_weight = 0;

  uint64_t i;
  for (i = 0; i < articles.size(); ++i) {
    auto& article = articles[i];
    current_weight += GetArticleWeight(article, signals);
    if (current_weight > picked_value) {
      break;
    }
  }

  mojom::ArticlePtr picked = std::move(articles[i]);
  articles.erase(articles.begin() + i);
  return picked;
}

}  // namespace

FeedV2Builder::FeedV2Builder(
    PublishersController& publishers_controller,
    ChannelsController& channels_controller,
    PrefService& prefs,
    history::HistoryService& history_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : fetcher_(publishers_controller, channels_controller, url_loader_factory),
      signal_calculator_(publishers_controller,
                         channels_controller,
                         prefs,
                         history_service) {}

FeedV2Builder::~FeedV2Builder() = default;

void FeedV2Builder::Build(BuildFeedCallback callback) {
  if (raw_feed_items_.size()) {
    BuildFeedFromArticles(std::move(callback));
    return;
  }

  FetchFeed(std::move(callback));
}

void FeedV2Builder::FetchFeed(BuildFeedCallback callback) {
  fetcher_.FetchFeed(
      base::BindOnce(&FeedV2Builder::OnFetchedFeed,
                     // Unretained is safe here because the FeedFetcher is owned
                     // by this and uses weak pointers internally.
                     base::Unretained(this), std::move(callback)));
}

void FeedV2Builder::OnFetchedFeed(BuildFeedCallback callback,
                                  FeedItems items,
                                  ETags tags) {
  raw_feed_items_ = std::move(items);
  CalculateSignals(std::move(callback));
}

void FeedV2Builder::CalculateSignals(BuildFeedCallback callback) {
  signal_calculator_.GetSignals(
      raw_feed_items_,
      base::BindOnce(
          &FeedV2Builder::OnCalculatedSignals,
          // Unretained is safe here because we own the SignalCalculator, and it
          // uses WeakPtr for its internal callbacks.
          base::Unretained(this), std::move(callback)));
}

void FeedV2Builder::OnCalculatedSignals(BuildFeedCallback callback,
                                        Signals signals) {
  signals_ = std::move(signals);
  BuildFeedFromArticles(std::move(callback));
}

void FeedV2Builder::BuildFeedFromArticles(BuildFeedCallback callback) {
  auto feed = mojom::FeedV2::New();

  std::vector<mojom::ArticlePtr> articles;
  std::vector<mojom::PromotedArticlePtr> ads;

  for (const auto& item : raw_feed_items_) {
    if (item.is_null()) {
      continue;
    }
    if (item->is_article()) {
      articles.push_back(item->get_article()->Clone());
    }
    if (item->is_promoted_article()) {
      ads.push_back(item->get_promoted_article()->Clone());
    }
  }

  mojom::ArticlePtr article;
  while ((article = PickRouletteAndRemove(articles, signals_))) {
    feed->items.push_back(mojom::FeedItemV2::NewArticle(std::move(article)));
  }

  std::move(callback).Run(std::move(feed));
}

}  // namespace brave_news
