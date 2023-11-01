// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_v2_builder.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/rand_util.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/browser/topics_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/features.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

namespace {

// An ArticleWeight has a few different components
struct ArticleWeight {
  // The pop_recency of the article. This is used for discover cards, where we
  // don't consider the subscription status or visit_weighting.
  double pop_recency = 0;

  // The complete weighting of the article, combining the pop_score,
  // visit_weighting & subscribed_weighting.
  double weighting = 0;

  // Whether the source which this article comes from has been visited. This
  // only considers Publishers, not Channels.
  bool visited = false;

  // Whether any sources/channels that could cause this article to be shown are
  // subscribed. At this point, disabled sources have already been filtered out.
  bool subscribed = false;
};

using ArticleInfo = std::tuple<mojom::FeedItemMetadataPtr, ArticleWeight>;
using ArticleInfos = std::vector<ArticleInfo>;

// Gets all relevant signals for an article.
// **Note:** Importantly, this function returns the Signal from the publisher
// first, and |GetArticleWeight| depends on this to determine whether the
// Publisher has been visited.
std::vector<mojom::Signal*> GetSignals(
    const std::string& locale,
    const mojom::FeedItemMetadataPtr& article,
    const Publishers& publishers,
    const Signals& signals) {
  std::vector<mojom::Signal*> result;
  auto it = signals.find(article->publisher_id);
  if (it != signals.end()) {
    result.push_back(it->second.get());
  }

  auto publisher_it = publishers.find(article->publisher_id);
  if (publisher_it == publishers.end()) {
    return result;
  }

  for (const auto& locale_info : publisher_it->second->locales) {
    if (locale_info->locale != locale) {
      continue;
    }
    for (const auto& channel : locale_info->channels) {
      auto signal_it = signals.find(channel);
      if (signal_it == signals.end()) {
        continue;
      }
      result.push_back(signal_it->second.get());
    }
  }
  return result;
}

double GetPopRecency(const mojom::FeedItemMetadataPtr& article) {
  const double pop_recency_half_life_in_hours =
      features::kBraveNewsPopScoreHalfLife.Get();

  auto& publish_time = article->publish_time;

  double popularity = article->pop_score == 0
                          ? features::kBraveNewsPopScoreFallback.Get()
                          : article->pop_score;
  double multiplier = publish_time > base::Time::Now() - base::Hours(5) ? 2 : 1;
  auto dt = base::Time::Now() - publish_time;

  return multiplier * popularity *
         pow(0.5, dt.InHours() / pop_recency_half_life_in_hours);
}

double GetSubscribedWeight(const mojom::FeedItemMetadataPtr& article,
                           const std::vector<mojom::Signal*>& signals) {
  return std::reduce(
      signals.begin(), signals.end(), 0.0, [](double prev, const auto* signal) {
        return prev + signal->subscribed_weight / signal->article_count;
      });
}

ArticleWeight GetArticleWeight(const mojom::FeedItemMetadataPtr& article,
                               const std::vector<mojom::Signal*>& signals) {
  // We should have at least one |Signal| from the |Publisher| for this source.
  CHECK(!signals.empty());

  const auto subscribed_weight = GetSubscribedWeight(article, signals);
  const double source_visits_min = features::kBraveNewsSourceVisitsMin.Get();
  const double source_visits_projected =
      source_visits_min + signals.at(0)->visit_weight * (1 - source_visits_min);
  const auto pop_recency = GetPopRecency(article);
  return {
      .pop_recency = pop_recency,
      .weighting = source_visits_projected * subscribed_weight * pop_recency,
      // Note: GetArticleWeight returns the Signal for the Publisher first, and
      // we use that to determine whether this Publisher has ever been visited.
      .visited = signals.at(0)->visit_weight != 0,
      .subscribed = subscribed_weight != 0,
  };
}

std::string PickRandom(const std::vector<std::string>& items) {
  CHECK(!items.empty());
  // Note: RandInt is inclusive, hence the minus 1
  return items[base::RandInt(0, items.size() - 1)];
}

ArticleInfos GetArticleInfos(const std::string& locale,
                             const FeedItems& feed_items,
                             const Publishers& publishers,
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

      auto article_signals =
          GetSignals(locale, article->data, publishers, signals);

      // If we don't have any signals for this article, or the source this
      // article comes from has been disabled then filter it out.
      if (article_signals.empty() ||
          base::ranges::any_of(article_signals, [](const auto* signal) {
            return signal->disabled;
          })) {
        continue;
      }

      ArticleInfo pair =
          std::tuple(article->data->Clone(),
                     GetArticleWeight(article->data, article_signals));
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

using GetWeighting = double(const ArticleWeight& weight);

// Picks an article with a probability article_weight/sum(article_weights).
mojom::FeedItemMetadataPtr PickRouletteAndRemove(
    ArticleInfos& articles,
    GetWeighting get_weighting = [](const auto& weight) {
      return weight.weighting;
    }) {
  double total_weight = 0;
  for (const auto& [article, weight] : articles) {
    total_weight += get_weighting(weight);
  }

  // None of the items are eligible to be picked.
  if (total_weight == 0) {
    return nullptr;
  }

  double picked_value = base::RandDouble() * total_weight;
  double current_weight = 0;

  uint64_t i;
  for (i = 0; i < articles.size(); ++i) {
    auto& [article, weight] = articles[i];
    current_weight += get_weighting(weight);
    if (current_weight > picked_value) {
      break;
    }
  }

  auto [article, weight] = std::move(articles[i]);
  articles.erase(articles.begin() + i);

  return std::move(article);
}

// Picking a discovery article works the same way as as a normal roulette
// selection, but we only consider articles that:
// 1. The user hasn't subscribed to.
// 2. **AND** The user hasn't visited.
mojom::FeedItemMetadataPtr PickDiscoveryArticleAndRemove(
    ArticleInfos& articles) {
  return PickRouletteAndRemove(articles, [](const auto& weight) {
    if (weight.subscribed || weight.visited) {
      return 0.;
    }
    return weight.pop_recency;
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

  const int block_min_inline = features::kBraveNewsMinBlockCards.Get();
  const int block_max_inline = features::kBraveNewsMaxBlockCards.Get();
  auto follow_count = GetNormal(block_min_inline, block_max_inline + 1);
  for (auto i = 0; i < follow_count; ++i) {
    // Ratio of inline articles to discovery articles.
    // discover ratio % of the time, we should do a discover card here instead
    // of a roulette card.
    // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.4rkb0vecgekl
    const double inline_discovery_ratio =
        features::kBraveNewsInlineDiscoveryRatio.Get();
    bool is_discover = base::RandDouble() < inline_discovery_ratio;
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
// https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.kxe6xeqm2vfn
std::vector<mojom::FeedItemV2Ptr> GenerateChannelBlock(
    const std::string& locale,
    const Publishers& publishers,
    const std::string& channel,
    ArticleInfos& articles) {
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
  result.push_back(mojom::FeedItemV2::NewCluster(mojom::Cluster::New(
      mojom::ClusterType::CHANNEL, channel, std::move(article_elements))));
  return result;
}

// Generate a Topic Cluster block
// https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.4vwmn4vmf2tq
std::vector<mojom::FeedItemV2Ptr> GenerateTopicBlock(
    const Publishers& publishers,
    base::span<const TopicAndArticles>& topics) {
  if (topics.empty()) {
    return {};
  }
  DVLOG(1) << __FUNCTION__;
  auto result = mojom::Cluster::New();
  result->type = mojom::ClusterType::TOPIC;

  auto& [topic, articles] = topics.front();
  result->id = topic.title;

  uint64_t max_articles = features::kBraveNewsMaxBlockCards.Get();
  for (const auto& article : articles) {
    auto item = mojom::FeedItemMetadata::New();
    auto id_it = base::ranges::find_if(publishers, [&article](const auto& p) {
      return p.second->publisher_name == article.publisher_name;
    });
    if (id_it != publishers.end()) {
      item->publisher_id = id_it->first;
    }
    item->publisher_name = article.publisher_name;
    item->category_name = article.category;
    item->description = article.description.value_or("");
    item->title = article.title;
    item->url = GURL(article.url);
    item->publish_time = base::Time::Now();
    item->image = mojom::Image::NewImageUrl(GURL(article.img.value_or("")));
    result->articles.push_back(mojom::ArticleElements::NewArticle(
        mojom::Article::New(std::move(item), false)));

    // For now, we truncate at |max_articles|. In future we may want to include
    // more articles here and have the option to show more in the front end.
    if (result->articles.size() >= max_articles) {
      break;
    }
  }

  // Make sure we don't reuse the topic by excluding it from our span.
  topics = base::make_span(std::next(topics.begin()), topics.end());

  std::vector<mojom::FeedItemV2Ptr> items;
  items.push_back(mojom::FeedItemV2::NewCluster(std::move(result)));
  return items;
}

// This function will generate a cluster block (if we have
// articles/topics/channels available).
// This could be either a Channel cluster or a Topic cluster, based on a ratio
// configured through the |kBraveNewsCategoryTopicRatio| FeatureParam.
// https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.agyx2d7gifd9
std::vector<mojom::FeedItemV2Ptr> GenerateClusterBlock(
    const std::string& locale,
    const Publishers& publishers,
    const std::vector<std::string>& channels,
    base::span<const TopicAndArticles>& topics,
    ArticleInfos& articles) {
  // If we have no channels, and no topics there's nothing we can do here.
  if (channels.empty() && topics.empty()) {
    DVLOG(1) << "Step 4: Nothing (no subscribed channels or unshown topics)";
    return {};
  }

  // Determine whether we should generate a channel or topic cluster.
  auto generate_channel =
      (!channels.empty() &&
       base::RandDouble() < features::kBraveNewsCategoryTopicRatio.Get()) ||
      topics.empty();

  if (generate_channel) {
    auto channel = PickRandom(channels);
    DVLOG(1) << "Step 4: Cluster Block (channel: " << channel << ")";
    return GenerateChannelBlock(locale, publishers, PickRandom(channels),
                                articles);
  } else {
    DVLOG(1) << "Step: Cluster Block (topic)";
    return GenerateTopicBlock(publishers, topics);
  }
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
        "", 0.0, 0.0, "Now");
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
      topics_fetcher_(url_loader_factory),
      signal_calculator_(publishers_controller,
                         channels_controller,
                         prefs,
                         history_service) {}

FeedV2Builder::~FeedV2Builder() = default;

void FeedV2Builder::BuildChannelFeed(const std::string& channel,
                                     BuildFeedCallback callback) {
  Build(
      /*recalculate_signals=*/true,
      base::BindOnce(
          [](base::WeakPtr<FeedV2Builder> builder, const std::string& channel,
             BuildFeedCallback callback, mojom::FeedV2Ptr _) {
            auto result = mojom::FeedV2::New();
            auto& publishers =
                builder->publishers_controller_->GetLastPublishers();
            auto& locale = builder->publishers_controller_->GetLastLocale();

            for (const auto& item : builder->raw_feed_items_) {
              if (!item->is_article()) {
                continue;
              }

              auto publisher_it =
                  publishers.find(item->get_article()->data->publisher_id);
              if (publisher_it == publishers.end()) {
                continue;
              }

              auto locale_info_it =
                  base::ranges::find_if(publisher_it->second->locales,
                                        [&locale](const auto& locale_info) {
                                          return locale == locale_info->locale;
                                        });
              if (locale_info_it == publisher_it->second->locales.end()) {
                continue;
              }

              if (!base::Contains(locale_info_it->get()->channels, channel)) {
                continue;
              }

              result->items.push_back(
                  mojom::FeedItemV2::NewArticle(item->get_article()->Clone()));
            }

            base::ranges::sort(result->items, [](const auto& a, const auto& b) {
              return GetPopRecency(b->get_article()->data) <
                     GetPopRecency(a->get_article()->data);
            });

            std::move(callback).Run(std::move(result));
          },
          weak_ptr_factory_.GetWeakPtr(), channel, std::move(callback)));
}

void FeedV2Builder::BuildPublisherFeed(const std::string& publisher_id,
                                       BuildFeedCallback callback) {
  Build(
      /*recalculate_signals=*/true,
      base::BindOnce(
          [](base::WeakPtr<FeedV2Builder> builder,
             const std::string& publisher_id, BuildFeedCallback callback,
             mojom::FeedV2Ptr _) {
            auto result = mojom::FeedV2::New();

            for (const auto& item : builder->raw_feed_items_) {
              if (!item->is_article()) {
                continue;
              }
              if (item->get_article()->data->publisher_id != publisher_id) {
                continue;
              }

              result->items.push_back(
                  mojom::FeedItemV2::NewArticle(item->get_article()->Clone()));
            }

            base::ranges::sort(result->items, [](const auto& a, const auto& b) {
              return b->get_article()->data->publish_time <
                     a->get_article()->data->publish_time;
            });

            std::move(callback).Run(std::move(result));
          },
          weak_ptr_factory_.GetWeakPtr(), publisher_id, std::move(callback)));
}

void FeedV2Builder::Build(bool recalculate_signals,
                          BuildFeedCallback callback) {
  pending_callbacks_.push_back(std::move(callback));

  if (is_building_) {
    return;
  }

  is_building_ = true;

  if (raw_feed_items_.size()) {
    if (recalculate_signals) {
      signals_.clear();
      CalculateSignals();
    } else {
      BuildFeedFromArticles();
    }
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
    Build(/*recalculate_signals=*/true, std::move(cb));
  } else {
    std::move(cb).Run(/*feed=*/nullptr);
  }
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

  GetTopics();
}

void FeedV2Builder::GetTopics() {
  DVLOG(1) << __FUNCTION__;
  topics_fetcher_.GetTopics(publishers_controller_->GetLastLocale(),
                            base::BindOnce(&FeedV2Builder::OnGotTopics,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void FeedV2Builder::OnGotTopics(TopicsResult topics) {
  DVLOG(1) << __FUNCTION__ << " (topic count: " << topics.size() << ")";
  topics_ = std::move(topics);

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
  auto articles =
      GetArticleInfos(locale, raw_feed_items_, publishers, signals_);
  auto feed = mojom::FeedV2::New();
  base::span<const TopicAndArticles> topics = base::make_span(topics_);

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
      GenerateChannelBlock(locale, publishers, kTopNewsChannel, articles);
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
      } else {
        items = GenerateClusterBlock(locale, publishers, subscribed_channels,
                                     topics, articles);
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
