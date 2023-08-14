// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_v2_builder.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <locale>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/cxx20_erase_vector.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/logging.h"
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

constexpr int kBlockInlineMin = 1;
constexpr int kBlockInlineMax = 5;
// constexpr double kInlineDiscoveryRatio = 0.25;
constexpr double kSourceSubscribedMin = 1e-6;
constexpr double kSourceSubscribedMax = 1;
constexpr double kSourceVisitsMin = 0.2;
constexpr double kPopRecencyHalfLifeInHours = 18;

bool TossCoin() {
  return base::RandDouble() < 0.5;
}

double GetPopRecency(const mojom::FeedItemMetadataPtr& article) {
  auto& publish_time = article->publish_time;

  // TODO(fallaciousreasoning): Use the new popularity field instead.
  double popularity = article->score == 0 ? 50 : article->score;
  double multiplier = publish_time > base::Time::Now() - base::Hours(5) ? 2 : 1;
  auto dt = base::Time::Now() - publish_time;

  return multiplier * popularity *
         pow(0.5, dt.InHours() / kPopRecencyHalfLifeInHours);
}

double GetArticleWeight(const mojom::FeedItemMetadataPtr& article,
                        const Signals& signals) {
  auto it = signals.find(article->url.spec());
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

std::string PickRandom(const std::vector<std::string>& items) {
  CHECK(!items.empty());
  return items[base::RandInt(0, items.size() - 1)];
}

mojom::FeedItemMetadataPtr PickRouletteAndRemove(
    std::vector<mojom::FeedItemMetadataPtr>& articles,
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

  auto picked = std::move(articles[i]);
  articles.erase(articles.begin() + i);
  return picked;
}

// Generates a standard block:
// 1. Hero Article
// 2. 1 - 5 following articles
std::vector<mojom::FeedItemV2Ptr> GenerateBlock(
    std::vector<mojom::FeedItemMetadataPtr>& articles,
    const Signals& signals) {
  DVLOG(1) << __FUNCTION__;
  std::vector<mojom::FeedItemV2Ptr> result;
  if (articles.empty()) {
    return result;
  }

  auto hero_article = PickRouletteAndRemove(articles, signals);
  if (!hero_article) {
    return result;
  }

  result.push_back(mojom::FeedItemV2::NewHero(
      mojom::HeroArticle::New(std::move(hero_article))));

  auto follow_count = base::RandInt(kBlockInlineMin, kBlockInlineMax);
  for (auto i = 0; i < follow_count; ++i) {
    auto generated = PickRouletteAndRemove(articles, signals);
    if (!generated) {
      continue;
    }
    result.push_back(mojom::FeedItemV2::NewArticle(
        mojom::Article::New(std::move(generated), false)));
  }

  return result;
}

std::vector<mojom::FeedItemV2Ptr> GenerateChannelBlock(
    std::vector<mojom::FeedItemMetadataPtr>& articles,
    const Signals& signals,
    const Publishers& publishers,
    const std::string& channel,
    const std::string& locale) {
  DVLOG(1) << __FUNCTION__;
  // First, create a set of all publishers in this channel.
  base::flat_set<std::string> allowed_publishers;
  for (const auto& [id, publisher] : publishers) {
    for (const auto& locale_info : publisher->locales) {
      if (locale != locale_info->locale) {
        continue;
      }

      if (base::ranges::any_of(locale_info->channels,
                               [&channel](const auto& publisher_channel) {
                                 return publisher_channel == channel;
                               })) {
        allowed_publishers.insert(id);
      }
    }
  }

  // now, filter articles to only include articles in the channel.
  std::vector<mojom::FeedItemMetadataPtr> allowed_articles;
  for (auto i = 0u; i < articles.size(); ++i) {
    auto& article = articles[i];
    if (!base::Contains(allowed_publishers, article->publisher_id)) {
      continue;
    }

    allowed_articles.push_back(std::move(article));
    articles.erase(articles.begin() + i);
    --i;
  }

  auto block = GenerateBlock(allowed_articles, signals);

  // Put the unused articles back in the original list.
  base::ranges::move(allowed_articles, std::back_inserter(articles));

  // If we didn't manage to generate a block, don't return any elements.
  if (block.empty()) {
    return {};
  }

  std::vector<mojom::ArticleElementsPtr> article_elements;
  for (const auto& item : block) {
    if (item->is_hero()) {
      article_elements.push_back(
          mojom::ArticleElements::NewHero(std::move(item->get_hero())));
    } else if (item->is_article()) {
      article_elements.push_back(
          mojom::ArticleElements::NewArticle(std::move(item->get_article())));
    }
  }

  std::vector<mojom::FeedItemV2Ptr> result;
  result.push_back(mojom::FeedItemV2::NewCluster(
      mojom::Cluster::New("channel", channel, std::move(article_elements))));
  return result;
}

std::vector<mojom::FeedItemV2Ptr> GenerateSpecialBlock(
    std::vector<std::string>& suggested_publisher_ids) {
  DVLOG(1) << __FUNCTION__;
  // Note: This step is not implemented properly yet. It should
  // 1. Display an advert, if we have one
  // 2. Fallback to a discover card.
  // Ads have not been integrated with this process yet, so this is being
  // mocked with another coin toss.
  std::vector<mojom::FeedItemV2Ptr> result;

  if (TossCoin()) {
    LOG(ERROR) << "Generated advert";
    auto metadata = mojom::FeedItemMetadata::New(
        "ad", base::Time::Now(), "Advert", "Some handy info",
        GURL("https://example.com"), "foo",
        mojom::Image::NewImageUrl(GURL("https://example.com/favicon.ico")), "",
        "", 0.0, "Now");
    result.push_back(mojom::FeedItemV2::NewAdvert(
        mojom::PromotedArticle::New(std::move(metadata), "test")));
  } else {
    if (!suggested_publisher_ids.empty()) {
      uint64_t preferred_count = 3;
      auto count = std::min(preferred_count, suggested_publisher_ids.size());
      std::vector<std::string> suggestions(
          suggested_publisher_ids.begin(),
          suggested_publisher_ids.begin() + count);

      // Remove the suggested publisher ids, so we don't suggest them again.
      suggested_publisher_ids.erase(suggested_publisher_ids.begin(),
                                    suggested_publisher_ids.begin() + count);

      LOG(ERROR) << "Generated discover card";
      result.push_back(mojom::FeedItemV2::NewDiscover(
          mojom::Discover::New(std::move(suggestions))));
    }
  }

  return result;
}

}  // namespace

FeedV2Builder::FeedV2Builder(
    PublishersController& publishers_controller,
    ChannelsController& channels_controller,
    SuggestionsController& suggestions_controller,
    PrefService& prefs,
    history::HistoryService& history_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : publishers_controller_(publishers_controller),
      channels_controller_(channels_controller),
      suggestions_controller_(suggestions_controller),
      prefs_(prefs),
      fetcher_(publishers_controller, channels_controller, url_loader_factory),
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
  DVLOG(1) << __FUNCTION__;
  fetcher_.FetchFeed(
      base::BindOnce(&FeedV2Builder::OnFetchedFeed,
                     // Unretained is safe here because the FeedFetcher is owned
                     // by this and uses weak pointers internally.
                     base::Unretained(this), std::move(callback)));
}

void FeedV2Builder::OnFetchedFeed(BuildFeedCallback callback,
                                  FeedItems items,
                                  ETags tags) {
  DVLOG(1) << __FUNCTION__;

  raw_feed_items_ = std::move(items);
  CalculateSignals(std::move(callback));
}

void FeedV2Builder::CalculateSignals(BuildFeedCallback callback) {
  DVLOG(1) << __FUNCTION__;

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
  DVLOG(1) << __FUNCTION__;

  signals_ = std::move(signals);
  GetSuggestedPublisherIds(std::move(callback));
}

void FeedV2Builder::GetSuggestedPublisherIds(BuildFeedCallback callback) {
  DVLOG(1) << __FUNCTION__;
  suggestions_controller_->GetSuggestedPublisherIds(
      base::BindOnce(&FeedV2Builder::OnGotSuggestedPublisherIds,
                     // todo: WEAKPTR
                     base::Unretained(this), std::move(callback)));
}

void FeedV2Builder::OnGotSuggestedPublisherIds(
    BuildFeedCallback callback,
    const std::vector<std::string>& suggested_ids) {
  DVLOG(1) << __FUNCTION__;
  suggested_publisher_ids_ = suggested_ids;
  BuildFeedFromArticles(std::move(callback));
}

void FeedV2Builder::BuildFeedFromArticles(BuildFeedCallback callback) {
  DVLOG(1) << __FUNCTION__;
  const auto& publishers = publishers_controller_->GetLastPublishers();
  const auto& locale = publishers_controller_->GetLastLocale();

  // Get a list of the subscribed channels - we'll use these when determining
  // what channel cards to show.
  Channels channels =
      channels_controller_->GetChannelsFromPublishers(publishers, &*prefs_);
  std::vector<std::string> subscribed_channels;
  for (const auto& [id, channel] : channels) {
    if (base::Contains(channel->subscribed_locales, locale)) {
      subscribed_channels.push_back(id);
    }
  }

  // Make a copy of these - we're going to edit the copy to prevent duplicates.
  std::vector<std::string> suggested_publisher_ids = suggested_publisher_ids_;

  auto feed = mojom::FeedV2::New();

  std::vector<mojom::FeedItemMetadataPtr> articles;
  base::flat_set<GURL> seen_articles;
  for (const auto& item : raw_feed_items_) {
    if (item.is_null()) {
      continue;
    }
    if (item->is_article()) {
      // Because we download feeds from multiple locales, it's possible there
      // will be duplicate articles, which we should filter out.
      if (seen_articles.contains(item->get_article()->data->url)) {
        continue;
      }
      seen_articles.insert(item->get_article()->data->url);

      articles.push_back(item->get_article()->data->Clone());
    }
  }

  auto add_items = [&feed](std::vector<mojom::FeedItemV2Ptr>& items) {
    base::ranges::move(items, std::back_inserter(feed->items));
  };

  // Step 1: Generate a block
  // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.rkq699fwps0
  auto initial_block = GenerateBlock(articles, signals_);
  add_items(initial_block);

  // Step 2: Generate a top news block
  // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.7z05nb4b269d
  auto top_news_block =
      GenerateChannelBlock(articles, signals_, publishers, "Top News", locale);
  add_items(top_news_block);

  // Repeat step 3 - 5 until we don't have any more articles to add to the feed.
  constexpr uint8_t kIterationTypes = 3;
  uint32_t iteration = 0;
  while (true) {
    std::vector<mojom::FeedItemV2Ptr> items;

    auto iteration_type = iteration % kIterationTypes;

    // Step 3: Block Generation
    // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.os2ze8cesd8v
    if (iteration_type == 0) {
      items = GenerateBlock(articles, signals_);
    }
    // Step 4: Block or Cluster Generation
    // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.tpvsjkq0lzmy
    else if (iteration_type == 1) {
      // Half the time, a normal block
      if (TossCoin()) {
        items = GenerateBlock(articles, signals_);
      }
      // If we have any subscribed channels, display a channel cluster.
      else if (!subscribed_channels.empty()) {
        // TODO(fallaciousreasoning): When we have topic support, add them here
        // too.
        items = GenerateChannelBlock(articles, signals_, publishers,
                                     PickRandom(subscribed_channels), locale);
      }
    }

    // Step 5: Optional special card
    // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.n1ipt86esc34
    else if (iteration_type == 2) {
      if (TossCoin()) {
        items = GenerateSpecialBlock(suggested_publisher_ids);
      }
    } else {
      NOTREACHED();
    }

    if (items.empty()) {
      break;
    }

    add_items(items);
    ++iteration;
  }

  std::vector<mojom::FeedItemV2Ptr> entries;
  while (!(entries = GenerateBlock(articles, signals_)).empty()) {
    add_items(entries);
  }

  std::move(callback).Run(std::move(feed));
}

}  // namespace brave_news
