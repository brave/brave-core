// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_v2_builder.h"

#include <stddef.h>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
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

// An ArticleInfo is a tuple of (item, signal, weighting). They're stored like
// this for performance, so we only need to calculate things once.
using ArticleInfo = std::tuple<mojom::FeedItemMetadataPtr, Signal, double>;
using ArticleInfos = std::vector<ArticleInfo>;

const Signal GetSignal(const mojom::FeedItemMetadataPtr& article,
                       const Signals& signals) {
  auto it = signals.find(article->publisher_id);
  if (it == signals.end()) {
    return nullptr;
  }
  return it->second->Clone();
}

double GetPopRecency(const mojom::FeedItemMetadataPtr& article) {
  // Every N hours the popRecency half life will halve. I.e, if this was 24,
  // every day the popularity score will be halved.
  constexpr double kPopRecencyHalfLifeInHours = 18;

  auto& publish_time = article->publish_time;

  // TODO(fallaciousreasoning): Use the new popularity field instead.
  // https://github.com/brave/brave-browser/issues/32173
  double popularity = article->score == 0 ? 50 : article->score;
  double multiplier = publish_time > base::Time::Now() - base::Hours(5) ? 2 : 1;
  auto dt = base::Time::Now() - publish_time;

  return multiplier * popularity *
         pow(0.5, dt.InHours() / kPopRecencyHalfLifeInHours);
}

double GetArticleWeight(const mojom::FeedItemMetadataPtr& article,
                        const Signal& signal) {
  // Weighting for unsubscribed sources. Small but non-zero, so we still include
  // them in the feed.
  constexpr double kSourceSubscribedMin = 1e-5;

  // Weighting for subscribed sources (either this or |kSourceSubscribedMin|
  // will be applied).
  constexpr double kSourceSubscribedMax = 1;

  // Multiplier for unvisited sources. |visit_weighting| is in the range [0, 1]
  // inclusive and will be shifted by this (i.e. [0, 1] ==> [0.2, 1.2]). This
  // ensures we still show unvisited sources in the feed.
  constexpr double kSourceVisitsMin = 0.2;

  double source_visits_projected =
      kSourceVisitsMin + signal->visit_weight * (1 - kSourceVisitsMin);
  double source_subscribed_projected =
      signal->subscribed ? kSourceSubscribedMax : kSourceSubscribedMin;
  return source_visits_projected * source_subscribed_projected *
         GetPopRecency(article);
}

std::string PickRandom(const std::vector<std::string>& items) {
  CHECK(!items.empty());
  // Note: RandInt is inclusive, hence the minus 1
  return items[base::RandInt(0, items.size() - 1)];
}

ArticleInfos GetArticleInfos(const FeedItems& feed_items,
                             const Signals& signals) {
  ArticleInfos articles;
  base::flat_set<GURL> seen_articles;
  for (const auto& item : feed_items) {
    if (item.is_null()) {
      continue;
    }
    if (item->is_article()) {
      // Because we download feeds from multiple locales, it's possible there
      // will be duplicate articles, which we should filter out.
      if (seen_articles.contains(item->get_article()->data->url)) {
        continue;
      }
      auto& article = item->get_article();

      seen_articles.insert(article->data->url);

      auto signal = GetSignal(article->data, signals);

      // If we don't have a signal for this article, filter it out.
      if (!signal) {
        continue;
      }

      std::tuple<mojom::FeedItemMetadataPtr, Signal, double> pair =
          std::tuple(article->data->Clone(), std::move(signal),
                     GetArticleWeight(article->data, signal));
      articles.push_back(std::move(pair));
    }
  }
  return articles;
}

// Randomly true/false with equal probability.
bool TossCoin() {
  return base::RandDouble() < 0.5;
}

// This is a Box Muller transform for getting a normally distributed value
// between [0, 1)
// https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
double GetNormal() {
  double u;
  double v;

  do {
    u = base::RandDouble();
  } while (u == 0);
  do {
    v = base::RandDouble();
  } while (v == 0);

  double result = sqrt(-2.0 * log(u)) * cos(2 * 3.1415 * v);
  result = result / 10 + 0.5;

  // Resample if outside the [0, 1] range
  if (result > 1 || result < 0) {
    return GetNormal();
  }

  return result;
}

// Returns a normally distributed value between min (inclusive) and max
// (exclusive).
int GetNormal(int min, int max) {
  return min + floor((max - min) * GetNormal());
}

using ShouldConsiderPredicate = bool(const Signal& signal);

// Picks an article with a probability article_weight/sum(article_weights).
mojom::FeedItemMetadataPtr PickRouletteAndRemove(
    ArticleInfos& articles,
    ShouldConsiderPredicate should_consider = [](const auto& signal) {
      return true;
    }) {
  double total_weight = 0;
  for (const auto& [article, signal, weight] : articles) {
    if (!should_consider(signal)) {
      continue;
    }
    total_weight += weight;
  }

  // None of the items are eligible to be picked.
  if (total_weight == 0) {
    return nullptr;
  }

  double picked_value = base::RandDouble() * total_weight;
  double current_weight = 0;

  uint64_t i;
  for (i = 0; i < articles.size(); ++i) {
    auto& [article, signal, weight] = articles[i];
    if (!should_consider(signal)) {
      continue;
    }

    current_weight += weight;
    if (current_weight > picked_value) {
      break;
    }
  }

  auto [article, signal, weight] = std::move(articles[i]);
  articles.erase(articles.begin() + i);

  return std::move(article);
}

// Picking a discovery article works the same way as as a normal roulette
// selection, but we only consider articles that:
// 1. The user hasn't subscribed to.
// 2. **AND** The user hasn't visited.
mojom::FeedItemMetadataPtr PickDiscoveryArticleAndRemove(
    ArticleInfos& articles) {
  return PickRouletteAndRemove(articles, [](const auto& signal) {
    return !signal->subscribed && signal->visit_weight == 0;
  });
}

// Generates a standard block:
// 1. Hero Article
// 2. 1 - 5 Inline Articles (a percentage of which might be discover cards).
std::vector<mojom::FeedItemV2Ptr> GenerateBlock(ArticleInfos& articles) {
  DVLOG(1) << __FUNCTION__;
  std::vector<mojom::FeedItemV2Ptr> result;
  if (articles.empty()) {
    return result;
  }

  auto hero_article = PickRouletteAndRemove(articles);
  if (!hero_article) {
    return result;
  }

  result.push_back(mojom::FeedItemV2::NewHero(
      mojom::HeroArticle::New(std::move(hero_article))));

  // Minimum number of articles in an inline block.
  constexpr int kBlockInlineMin = 1;

  // Maximum number of articles in an inline block.
  constexpr int kBlockInlineMax = 5;
  auto follow_count = GetNormal(kBlockInlineMin, kBlockInlineMax + 1);
  for (auto i = 0; i < follow_count; ++i) {
    // Ratio of inline articles to discovery articles.
    // discover ratio % of the time, we should do a discover card here instead
    // of a roulette card.
    // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.4rkb0vecgekl
    constexpr double kInlineDiscoveryRatio = 0.25;
    bool is_discover = base::RandDouble() < kInlineDiscoveryRatio;
    auto generated = is_discover ? PickDiscoveryArticleAndRemove(articles)
                                 : PickRouletteAndRemove(articles);
    if (!generated) {
      continue;
    }
    result.push_back(mojom::FeedItemV2::NewArticle(
        mojom::Article::New(std::move(generated), is_discover)));
  }

  return result;
}

// Generates a Channel Block
// 1 Hero Articles (from the channel)
// 1 - 5 Inline Articles (from the channel)
// This function is the same as GenerateBlock, except that the available
// articles are filtered to only be from the specified channel.
std::vector<mojom::FeedItemV2Ptr> GenerateChannelBlock(
    ArticleInfos& articles,
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
  ArticleInfos allowed_articles;
  for (auto i = 0u; i < articles.size(); ++i) {
    auto& article_info = articles[i];
    if (!base::Contains(allowed_publishers,
                        std::get<0>(article_info)->publisher_id)) {
      continue;
    }

    allowed_articles.push_back(std::move(article_info));
    articles.erase(articles.begin() + i);
    --i;
  }

  auto block = GenerateBlock(allowed_articles);

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

// Generates a "Special Block" this will be one of the following:
// 1. An advert, if we have one
// 2. A "Discover" entry, which suggests some more publishers for the user to
// subscribe to.
// 3. Nothing.
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
    DVLOG(1) << "Generating advert";
    auto metadata = mojom::FeedItemMetadata::New(
        "ad", base::Time::Now(), "Advert", "Some handy info",
        GURL("https://example.com"), "foo",
        mojom::Image::NewImageUrl(GURL("https://example.com/favicon.ico")), "",
        "", 0.0, "Now");
    result.push_back(mojom::FeedItemV2::NewAdvert(
        mojom::PromotedArticle::New(std::move(metadata), "test")));
  } else {
    if (!suggested_publisher_ids.empty()) {
      size_t preferred_count = 3;
      auto count = std::min(preferred_count, suggested_publisher_ids.size());
      std::vector<std::string> suggestions(
          suggested_publisher_ids.begin(),
          suggested_publisher_ids.begin() + count);

      // Remove the suggested publisher ids, so we don't suggest them again.
      suggested_publisher_ids.erase(suggested_publisher_ids.begin(),
                                    suggested_publisher_ids.begin() + count);

      DVLOG(1) << "Generating publisher suggestions (discover)";
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
  pending_callbacks_.push_back(std::move(callback));

  if (is_building_) {
    return;
  }

  is_building_ = true;

  if (raw_feed_items_.size()) {
    BuildFeedFromArticles();
    return;
  }

  FetchFeed();
}

void FeedV2Builder::GetSignals(GetSignalsCallback callback) {
  auto cb = base::BindOnce(
      [](FeedV2Builder* builder, GetSignalsCallback callback,
         mojom::FeedV2Ptr _) {
        base::flat_map<std::string, Signal> signals;
        for (const auto& [key, value] : builder->signals_) {
          signals[key] = value->Clone();
        }
        std::move(callback).Run(std::move(signals));
      },
      this, std::move(callback));
  if (signals_.empty()) {
    Build(std::move(cb));
    return;
  }
  std::move(cb).Run(/* feed */ nullptr);
}

void FeedV2Builder::FetchFeed() {
  DVLOG(1) << __FUNCTION__;
  fetcher_.FetchFeed(
      base::BindOnce(&FeedV2Builder::OnFetchedFeed,
                     // Unretained is safe here because the FeedFetcher is owned
                     // by this and uses weak pointers internally.
                     base::Unretained(this)));
}

void FeedV2Builder::OnFetchedFeed(FeedItems items, ETags tags) {
  DVLOG(1) << __FUNCTION__;

  raw_feed_items_ = std::move(items);
  CalculateSignals();
}

void FeedV2Builder::CalculateSignals() {
  DVLOG(1) << __FUNCTION__;

  signal_calculator_.GetSignals(
      raw_feed_items_,
      base::BindOnce(
          &FeedV2Builder::OnCalculatedSignals,
          // Unretained is safe here because we own the SignalCalculator, and it
          // uses WeakPtr for its internal callbacks.
          base::Unretained(this)));
}

void FeedV2Builder::OnCalculatedSignals(Signals signals) {
  DVLOG(1) << __FUNCTION__;

  signals_ = std::move(signals);
  GetSuggestedPublisherIds();
}

void FeedV2Builder::GetSuggestedPublisherIds() {
  DVLOG(1) << __FUNCTION__;
  suggestions_controller_->GetSuggestedPublisherIds(
      base::BindOnce(&FeedV2Builder::OnGotSuggestedPublisherIds,
                     weak_ptr_factory_.GetWeakPtr()));
}

void FeedV2Builder::OnGotSuggestedPublisherIds(
    const std::vector<std::string>& suggested_ids) {
  DVLOG(1) << __FUNCTION__;
  suggested_publisher_ids_ = suggested_ids;
  BuildFeedFromArticles();
}

void FeedV2Builder::BuildFeedFromArticles() {
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
  auto suggested_publisher_ids = suggested_publisher_ids_;
  auto articles = GetArticleInfos(raw_feed_items_, signals_);
  auto feed = mojom::FeedV2::New();

  auto add_items = [&feed](std::vector<mojom::FeedItemV2Ptr>& items) {
    base::ranges::move(items, std::back_inserter(feed->items));
  };

  // Step 1: Generate a block
  // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.rkq699fwps0
  auto initial_block = GenerateBlock(articles);
  DVLOG(1) << "Step 1: Standard Block (" << initial_block.size()
           << " articles)";
  add_items(initial_block);

  // Step 2: Generate a top news block
  // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.7z05nb4b269d
  auto top_news_block =
      GenerateChannelBlock(articles, publishers, kTopNewsChannel, locale);
  DVLOG(1) << "Step 2: Top News Block";
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
      DVLOG(1) << "Step 3: Standard Block";
      items = GenerateBlock(articles);
    } else if (iteration_type == 1) {
      // Step 4: Block or Cluster Generation
      // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.tpvsjkq0lzmy
      // Half the time, a normal block
      if (TossCoin()) {
        DVLOG(1) << "Step 4: Standard Block";
        items = GenerateBlock(articles);
      } else if (!subscribed_channels.empty()) {
        // If we have any subscribed channels, display a channel cluster.
        // TODO(fallaciousreasoning): When we have topic support, add them here
        // too.
        auto channel = PickRandom(subscribed_channels);
        DVLOG(1) << "Step 4: Cluster Block (channel: " << channel << ")";
        items = GenerateChannelBlock(articles, publishers, channel, locale);
      } else {
        DVLOG(1) << "Step 4: Nothing (no subscribed channels)";
      }
    } else if (iteration_type == 2) {
      // Step 5: Optional special card
      // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.n1ipt86esc34
      if (TossCoin()) {
        DVLOG(1) << "Step 5: Special Block";
        items = GenerateSpecialBlock(suggested_publisher_ids);
      } else {
        DVLOG(1) << "Step 5: None (approximately half the time)";
      }
    } else {
      NOTREACHED();
    }

    // If we couldn't generate a normal block, break.
    if (iteration_type == 0 && items.empty()) {
      break;
    }

    DVLOG(1) << "Adding " << items.size() << " new articles (iteration type is "
             << iteration_type << "). Currently have " << feed->items.size()
             << " articles";
    add_items(items);
    ++iteration;
  }

  // Fire all the pending callbacks.
  for (auto& callback : pending_callbacks_) {
    std::move(callback).Run(feed->Clone());
  }

  pending_callbacks_.clear();
  is_building_ = false;
}

}  // namespace brave_news
