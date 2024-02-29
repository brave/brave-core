// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_v2_builder.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/rand_util.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/components/brave_news/api/topics.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/browser/topics_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/features.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

namespace {

/* publisher_or_channel_id, is_channel */
using ContentGroup = std::pair<std::string, bool>;
constexpr char kAllContentGroup[] = "all";
constexpr float kSampleContentGroupAllRatio = 0.2f;

// Returns a tuple of the feed hash and the number of subscribed publishers.
std::tuple<std::string, size_t> GetFeedHashAndSubscribedCount(
    const Channels& channels,
    const Publishers& publishers,
    const ETags& etags) {
  std::vector<std::string> hash_items;
  size_t subscribed_count = 0;

  for (const auto& [channel_id, channel] : channels) {
    if (!channel->subscribed_locales.empty()) {
      hash_items.push_back(channel_id);
      subscribed_count++;
    }
  }

  for (const auto& [id, publisher] : publishers) {
    if (publisher->user_enabled_status == mojom::UserEnabled::ENABLED ||
        publisher->type == mojom::PublisherType::DIRECT_SOURCE) {
      hash_items.push_back(id);
      subscribed_count++;
    }

    // Disabling a publisher should also change the hash, as it will affect what
    // articles can be shown.
    if (publisher->user_enabled_status == mojom::UserEnabled::DISABLED) {
      hash_items.push_back(id + "_disabled");
    }
  }

  for (const auto& [region, etag] : etags) {
    hash_items.push_back(region + etag);
  }

  std::hash<std::string> hasher;
  std::string hash;

  for (const auto& hash_item : hash_items) {
    hash = base::NumberToString(hasher(hash + hash_item));
  }

  return {hash, subscribed_count};
}

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

  double popularity = std::min(article->pop_score, 100.0) / 100.0 +
                      features::kBraveNewsPopScoreMin.Get();
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
      .weighting = source_visits_projected + subscribed_weight + pop_recency,
      // Note: GetArticleWeight returns the Signal for the Publisher first, and
      // we use that to determine whether this Publisher has ever been visited.
      .visited = signals.at(0)->visit_weight != 0,
      .subscribed = subscribed_weight != 0,
  };
}

template <typename T>
T PickRandom(const std::vector<T>& items) {
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

std::vector<std::string> GetChannelsForPublisher(
    const std::string& locale,
    const mojom::PublisherPtr& publisher) {
  std::vector<std::string> result;
  for (const auto& locale_info : publisher->locales) {
    if (locale_info->locale != locale) {
      continue;
    }
    for (const auto& channel : locale_info->channels) {
      result.push_back(channel);
    }
  }
  return result;
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

using GetWeighting =
    base::RepeatingCallback<double(const mojom::FeedItemMetadataPtr& metadata,
                                   const ArticleWeight& weight)>;

// Sample across subscribed channels (direct and native) and publishers.
ContentGroup SampleContentGroup(
    const std::vector<ContentGroup>& eligible_content_groups) {
  ContentGroup sampled_content_group;
  if (eligible_content_groups.empty()) {
    return sampled_content_group;
  }

  if (base::RandDouble() < kSampleContentGroupAllRatio) {
    return std::make_pair(kAllContentGroup, true);
  }
  return PickRandom<ContentGroup>(eligible_content_groups);
}

// Picks the first article from the list - useful when the list has been sorted.
int PickFirstIndex(const ArticleInfos& articles) {
  return articles.empty() ? -1 : 0;
}

// Picks an article with a probability article_weight/sum(article_weights).
int PickRouletteWithWeighting(const ArticleInfos& articles,
                              GetWeighting get_weighting) {
  std::vector<double> weights;
  base::ranges::transform(articles, std::back_inserter(weights),
                          [&get_weighting](const auto& article_info) {
                            return get_weighting.Run(std::get<0>(article_info),
                                                     std::get<1>(article_info));
                          });

  // None of the items are eligible to be picked.
  const auto total_weight =
      std::accumulate(weights.begin(), weights.end(), 0.0);
  if (total_weight == 0) {
    return -1;
  }

  double picked_value = base::RandDouble() * total_weight;
  double current_weight = 0;

  uint64_t i;
  for (i = 0; i < weights.size(); ++i) {
    current_weight += weights[i];
    if (current_weight > picked_value) {
      break;
    }
  }

  return i;
}

int PickRoulette(const ArticleInfos& articles) {
  return PickRouletteWithWeighting(
      articles,
      base::BindRepeating([](const mojom::FeedItemMetadataPtr& metadata,
                             const ArticleWeight& weight) {
        return weight.subscribed ? weight.weighting : 0;
      }));
}

mojom::FeedItemMetadataPtr PickAndRemove(ArticleInfos& articles,
                                         PickArticles picker) {
  auto index = picker.Run(articles);
  if (index == -1) {
    return nullptr;
  }

  auto [article, weight] = std::move(articles[index]);
  articles.erase(articles.begin() + index);

  return std::move(article);
}

// Picking a discovery article works the same way as as a normal roulette
// selection, but we only consider articles that:
// 1. The user hasn't subscribed to.
// 2. **AND** The user hasn't visited.
mojom::FeedItemMetadataPtr PickDiscoveryArticleAndRemove(
    ArticleInfos& articles) {
  PickArticles pick = base::BindRepeating([](const ArticleInfos& articles) {
    return PickRouletteWithWeighting(
        articles,
        base::BindRepeating([](const mojom::FeedItemMetadataPtr& metadata,
                               const ArticleWeight& weight) {
          if (weight.subscribed) {
            return 0.0;
          }
          return weight.pop_recency;
        }));
  });
  return PickAndRemove(articles, pick);
}

// Generates a standard block:
// 1. Hero Article
// 2. 1 - 5 Inline Articles (a percentage of which might be discover cards).
std::vector<mojom::FeedItemV2Ptr> GenerateBlock(ArticleInfos& articles,
                                                PickArticles hero_picker,
                                                PickArticles article_picker,
                                                double inline_discovery_ratio) {
  DVLOG(1) << __FUNCTION__;
  std::vector<mojom::FeedItemV2Ptr> result;
  if (articles.empty()) {
    return result;
  }

  auto hero_article = PickAndRemove(articles, hero_picker);

  // We might not be able to generate a hero card, if none of the articles in
  // this feed have an image.
  if (hero_article) {
    result.push_back(mojom::FeedItemV2::NewHero(
        mojom::HeroArticle::New(std::move(hero_article))));
  }

  const int block_min_inline = features::kBraveNewsMinBlockCards.Get();
  const int block_max_inline = features::kBraveNewsMaxBlockCards.Get();
  auto follow_count = GetNormal(block_min_inline, block_max_inline + 1);
  for (auto i = 0; i < follow_count; ++i) {
    bool is_discover = base::RandDouble() < inline_discovery_ratio;
    mojom::FeedItemMetadataPtr generated;

    if (is_discover) {
      generated = PickDiscoveryArticleAndRemove(articles);
    } else {
      generated = PickAndRemove(articles, article_picker);
    }

    if (!generated) {
      DVLOG(1) << "Failed to generate article (is_discover=" << is_discover
               << ")";
      continue;
    }
    result.push_back(mojom::FeedItemV2::NewArticle(
        mojom::Article::New(std::move(generated), is_discover)));
  }

  return result;
}

std::vector<mojom::FeedItemV2Ptr> GenerateBlock(
    ArticleInfos& articles,
    // Ratio of inline articles to discovery articles.
    // discover ratio % of the time, we should do a discover card here instead
    // of a roulette card.
    // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.4rkb0vecgekl
    double inline_discovery_ratio =
        features::kBraveNewsInlineDiscoveryRatio.Get()) {
  PickArticles pick_hero = base::BindRepeating([](const ArticleInfos& infos) {
    return PickRouletteWithWeighting(
        infos,
        base::BindRepeating([](const mojom::FeedItemMetadataPtr& metadata,
                               const ArticleWeight& weight) {
          auto image_url = metadata->image->is_padded_image_url()
                               ? metadata->image->get_padded_image_url()
                               : metadata->image->get_image_url();
          return image_url.is_valid() && weight.subscribed ? weight.weighting
                                                           : 0;
        }));
  });

  return GenerateBlock(articles, std::move(pick_hero),
                       base::BindRepeating(&PickRoulette),
                       inline_discovery_ratio);
}

// Generates a block from sampled content groups:
// 1. Hero Article
// 2. 1 - 5 Inline Articles (a percentage of which might be discover cards).
std::vector<mojom::FeedItemV2Ptr> GenerateBlockFromContentGroups(
    ArticleInfos& articles,
    const std::string& locale,
    const Publishers& publishers,
    const std::vector<ContentGroup>& eligible_content_groups,
    // Ratio of inline articles to discovery articles.
    // discover ratio % of the time, we should do a discover card here instead
    // of a roulette card.
    // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.4rkb0vecgekl
    double inline_discovery_ratio =
        features::kBraveNewsInlineDiscoveryRatio.Get()) {
  DVLOG(1) << __FUNCTION__;
  std::vector<mojom::FeedItemV2Ptr> result;
  if (articles.empty() || eligible_content_groups.empty()) {
    return result;
  }

  base::flat_map<std::string, std::vector<std::string>>
      publisher_id_to_channels;
  for (const auto& [publisher_id, publisher] : publishers) {
    publisher_id_to_channels[publisher_id] =
        GetChannelsForPublisher(locale, publisher);
  }

  // Generates a GetWeighting function tied to a specific content group. Each
  // invocation of |get_weighting| will generate a new |GetWeighting| tied to a
  // (freshly sampled) content_group.
  auto get_weighting = base::BindRepeating(
      [](const std::string& locale,
         std::vector<ContentGroup> eligible_content_groups,
         base::flat_map<std::string, std::vector<std::string>>
             publisher_id_to_channels,
         bool is_hero) {
        return base::BindRepeating(
            [](const bool is_hero, const ContentGroup& content_group,
               const base::flat_map<std::string, std::vector<std::string>>&
                   publisher_id_to_channels,
               const std::string& locale,
               const mojom::FeedItemMetadataPtr& metadata,
               const ArticleWeight& weight) {
              if (is_hero) {
                auto image_url = metadata->image->is_padded_image_url()
                                     ? metadata->image->get_padded_image_url()
                                     : metadata->image->get_image_url();
                if (!image_url.is_valid()) {
                  return 0.0;
                }
              }

              if (/*is_channel*/ content_group.second &&
                  content_group.first != kAllContentGroup) {
                auto channels =
                    publisher_id_to_channels.find(metadata->publisher_id);
                if (base::Contains(channels->second, content_group.first)) {
                  return weight.weighting;
                }

                return 0.0;
              } else if (/*is_channel*/ !content_group.second) {
                return metadata->publisher_id == content_group.first
                           ? weight.weighting
                           : 0.0;
              }

              return weight.weighting;
            },
            is_hero, SampleContentGroup(eligible_content_groups),
            publisher_id_to_channels, locale);
      },
      locale, std::move(eligible_content_groups),
      std::move(publisher_id_to_channels));

  PickArticles pick_hero = base::BindRepeating(
      [](GetWeighting weighting, const ArticleInfos& articles) {
        return PickRouletteWithWeighting(articles, std::move(weighting));
      },
      get_weighting.Run(true));

  PickArticles pick_article = base::BindRepeating(
      [](base::RepeatingCallback<GetWeighting(bool)> gen_weighting,
         const ArticleInfos& articles) {
        return PickRouletteWithWeighting(articles, gen_weighting.Run(false));
      },
      std::move(get_weighting));

  return GenerateBlock(articles, pick_hero, pick_article,
                       inline_discovery_ratio);
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

  auto block = GenerateBlock(allowed_articles, /*inline_discovery_ratio=*/0);

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

mojom::FeedItemMetadataPtr FromTopicArticle(
    const Publishers& publishers,
    const api::topics::TopicArticle& article) {
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
  return item;
}

// We use this for the Top News cluster, at the start of the feed, to match
// (more or less) what Brave Search does.
std::vector<mojom::FeedItemV2Ptr> GenerateTopTopicsBlock(
    const Publishers& publishers,
    const base::span<const TopicAndArticles>& topics) {
  if (topics.empty()) {
    return {};
  }

  std::vector<mojom::ArticleElementsPtr> items;
  const auto max_block_size =
      static_cast<size_t>(features::kBraveNewsMaxBlockCards.Get());
  for (const auto& [topic, articles] : topics) {
    if (articles.empty()) {
      continue;
    }

    auto item = FromTopicArticle(publishers, articles.at(0));
    items.push_back(mojom::ArticleElements::NewArticle(
        mojom::Article::New(std::move(item), false)));
    if (items.size() >= max_block_size) {
      break;
    }
  }
  std::vector<mojom::FeedItemV2Ptr> result;
  result.push_back(mojom::FeedItemV2::NewCluster(mojom::Cluster::New(
      mojom::ClusterType::TOPIC, kTopNewsChannel, std::move(items))));
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
  result->id = topic.claude_title_short;

  uint64_t max_articles = features::kBraveNewsMaxBlockCards.Get();
  for (const auto& article : articles) {
    auto item = FromTopicArticle(publishers, article);
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
    DVLOG(1) << "Nothing (no subscribed channels or unshown topics)";
    return {};
  }

  // Determine whether we should generate a channel or topic cluster.
  auto generate_channel =
      (!channels.empty() &&
       base::RandDouble() < features::kBraveNewsCategoryTopicRatio.Get()) ||
      topics.empty();

  if (generate_channel) {
    auto channel = PickRandom(channels);
    DVLOG(1) << "Cluster Block (channel: " << channel << ")";
    return GenerateChannelBlock(locale, publishers, PickRandom(channels),
                                articles);
  } else {
    DVLOG(1) << "Cluster Block (topic)";
    return GenerateTopicBlock(publishers, topics);
  }
}

std::vector<mojom::FeedItemV2Ptr> GenerateAd() {
  DVLOG(1) << __FUNCTION__;
  std::vector<mojom::FeedItemV2Ptr> result;
  result.push_back(mojom::FeedItemV2::NewAdvert(mojom::FeedV2Ad::New()));
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

  if (TossCoin()) {
    return GenerateAd();
  }

  std::vector<mojom::FeedItemV2Ptr> result;
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

  return result;
}

}  // namespace

struct FeedV2Builder::FeedGenerationInfo {
  std::string locale;
  FeedItems feed_items;
  Publishers publishers;
  std::vector<std::string> channels;
  Signals signals;
  std::vector<std::string> suggested_publisher_ids;
  TopicsResult topics;
  FeedGenerationInfo(const std::string& locale,
                     const FeedItems& feed_items,
                     const Publishers& publishers,
                     std::vector<std::string> channels,
                     const Signals& signals,
                     const std::vector<std::string>& suggested_publisher_ids,
                     const TopicsResult& topics)
      : locale(locale),
        channels(std::move(channels)),
        suggested_publisher_ids(suggested_publisher_ids) {
    this->feed_items.reserve(feed_items.size());
    for (const auto& item : feed_items) {
      this->feed_items.push_back(item->Clone());
    }

    this->publishers.reserve(publishers.size());
    for (const auto& [id, publisher] : publishers) {
      this->publishers[id] = publisher.Clone();
    }

    this->signals.reserve(signals.size());
    for (const auto& [id, signal] : signals) {
      this->signals[id] = signal->Clone();
    }

    this->topics.reserve(topics.size());
    for (const auto& topic : topics) {
      std::vector<api::topics::TopicArticle> articles;
      articles.reserve(topic.second.size());
      for (const auto& article : topic.second) {
        articles.push_back(article.Clone());
      }
      this->topics.emplace_back(topic.first.Clone(), std::move(articles));
    }
  }
  FeedGenerationInfo(const FeedGenerationInfo&) = delete;
  FeedGenerationInfo& operator=(const FeedGenerationInfo&) = delete;
  FeedGenerationInfo(FeedGenerationInfo&&) = default;
  FeedGenerationInfo& operator=(FeedGenerationInfo&&) = default;
  ~FeedGenerationInfo() = default;
};

FeedV2Builder::UpdateRequest::UpdateRequest(
    BraveNewsSubscriptions subscriptions,
    UpdateSettings settings,
    UpdateCallback callback)
    : settings(std::move(settings)), subscriptions(std::move(subscriptions)) {
  callbacks.push_back(std::move(callback));
}
FeedV2Builder::UpdateRequest::~UpdateRequest() = default;
FeedV2Builder::UpdateRequest::UpdateRequest(UpdateRequest&&) = default;
FeedV2Builder::UpdateRequest& FeedV2Builder::UpdateRequest::operator=(
    UpdateRequest&&) = default;
bool FeedV2Builder::UpdateRequest::IsSufficient(
    const UpdateSettings& other_settings) {
  return !(
      (other_settings.feed && !settings.feed) ||
      (other_settings.signals && !settings.signals) ||
      (other_settings.suggested_publishers && !settings.suggested_publishers) ||
      (other_settings.topics && !settings.topics));
}

void FeedV2Builder::UpdateRequest::AlsoUpdate(
    const UpdateSettings& other_settings,
    UpdateCallback callback) {
  settings.feed |= other_settings.feed;
  settings.signals |= other_settings.signals;
  settings.suggested_publishers |= other_settings.suggested_publishers;
  settings.topics |= other_settings.suggested_publishers;
  callbacks.push_back(std::move(callback));
}

// static
mojom::FeedV2Ptr FeedV2Builder::GenerateBasicFeed(FeedGenerationInfo info,
                                                  PickArticles pick_hero,
                                                  PickArticles pick_article) {
  DVLOG(1) << __FUNCTION__;
  DCHECK(!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  auto articles = GetArticleInfos(info.locale, info.feed_items, info.publishers,
                                  info.signals);

  auto feed = mojom::FeedV2::New();

  constexpr size_t kIterationsPerAd = 2;
  size_t blocks = 0;
  while (!articles.empty()) {
    auto items = GenerateBlock(articles, pick_hero, pick_article,
                               /*inline_discovery_ratio=*/0);
    if (items.empty()) {
      break;
    }

    // After the first block, every second block should be an ad.
    if (blocks % kIterationsPerAd == 0 && blocks != 0) {
      std::ranges::move(GenerateSpecialBlock(info.suggested_publisher_ids),
                        std::back_inserter(items));
    }

    std::ranges::move(items, std::back_inserter(feed->items));
    blocks++;
  }

  // Insert an ad as the second item.
  if (feed->items.size() > 1) {
    auto ad = GenerateAd();
    feed->items.insert(feed->items.begin() + 1,
                       std::make_move_iterator(ad.begin()),
                       std::make_move_iterator(ad.end()));
  }

  return feed;
}

// static
mojom::FeedV2Ptr FeedV2Builder::GenerateAllFeed(FeedGenerationInfo info) {
  DVLOG(1) << __FUNCTION__;
  DCHECK(!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  // Make a copy of these - we're going to edit the copy to prevent duplicates.
  auto articles = GetArticleInfos(info.locale, info.feed_items, info.publishers,
                                  info.signals);
  auto feed = mojom::FeedV2::New();

  base::span<const TopicAndArticles> topics_span = base::make_span(info.topics);

  auto add_items = [&feed](std::vector<mojom::FeedItemV2Ptr>& items) {
    base::ranges::move(items, std::back_inserter(feed->items));
  };

  std::vector<ContentGroup> eligible_content_groups;
  for (const auto& channel_id : info.channels) {
    eligible_content_groups.push_back(std::make_pair(channel_id, true));
  }
  for (const auto& [publisher_id, publisher] : info.publishers) {
    if (publisher->user_enabled_status == mojom::UserEnabled::ENABLED ||
        publisher->type == mojom::PublisherType::DIRECT_SOURCE) {
      eligible_content_groups.push_back(std::make_pair(publisher_id, false));
      DVLOG(1) << "Subscribed to publisher: " << publisher->publisher_name;
    }
  }

  // If we aren't subscribed to anything, or we failed to fetch any articles
  // from the internet, don't try and generate a feed.
  if (eligible_content_groups.size() == 0 || info.feed_items.size() == 0) {
    return feed;
  }

  // Step 1: Generate a block
  // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.rkq699fwps0
  std::vector<mojom::FeedItemV2Ptr> initial_block =
      GenerateBlockFromContentGroups(articles, info.locale, info.publishers,
                                     eligible_content_groups);
  DVLOG(1) << "Step 1: Standard Block (" << initial_block.size()
           << " articles)";
  add_items(initial_block);

  // Step 2: We always add an advertisment after the first block.
  // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.82154jsxm16
  auto advert = GenerateAd();
  DVLOG(1) << "Step 2: Advertisement";
  add_items(advert);

  // Step 3: Generate a top news block
  // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.7z05nb4b269d
  // This block is a bit special - we take the top articles from the top few
  // topics and display them in a cluster. If there are no topics, we try and do
  // the same thing, but with the Top News channel.
  auto top_news_block = GenerateTopTopicsBlock(info.publishers, topics_span);
  if (top_news_block.empty()) {
    top_news_block = GenerateChannelBlock(info.locale, info.publishers,
                                          kTopNewsChannel, articles);
  }
  DVLOG(1) << "Step 3: Top News Block";
  add_items(top_news_block);

  // Repeat step 4 - 6 until we don't have any more articles to add to the feed.
  constexpr uint8_t kIterationTypes = 3;
  uint32_t iteration = 0;
  while (true) {
    std::vector<mojom::FeedItemV2Ptr> items;

    auto iteration_type = iteration % kIterationTypes;

    // Step 4: Block Generation
    // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.os2ze8cesd8v
    if (iteration_type == 0) {
      DVLOG(1) << "Step 4: Standard Block";
      items = GenerateBlockFromContentGroups(
          articles, info.locale, info.publishers, eligible_content_groups);
    } else if (iteration_type == 1) {
      // Step 5: Block or Cluster Generation
      // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.tpvsjkq0lzmy
      // Half the time, a normal block
      if (TossCoin()) {
        DVLOG(1) << "Step 5: Standard Block";
        items = GenerateBlockFromContentGroups(
            articles, info.locale, info.publishers, eligible_content_groups);
      } else {
        items = GenerateClusterBlock(info.locale, info.publishers,
                                     info.channels, topics_span, articles);
      }
    } else if (iteration_type == 2) {
      // Step 6: Optional special card
      // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.n1ipt86esc34
      if (TossCoin()) {
        DVLOG(1) << "Step 6: Special Block";
        items = GenerateSpecialBlock(info.suggested_publisher_ids);
      } else {
        DVLOG(1) << "Step 6: None (approximately half the time)";
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

  return feed;
}

FeedV2Builder::FeedV2Builder(
    PublishersController& publishers_controller,
    ChannelsController& channels_controller,
    SuggestionsController& suggestions_controller,
    history::HistoryService& history_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : publishers_controller_(publishers_controller),
      channels_controller_(channels_controller),
      suggestions_controller_(suggestions_controller),
      fetcher_(publishers_controller, url_loader_factory),
      topics_fetcher_(url_loader_factory),
      signal_calculator_(publishers_controller,
                         channels_controller,
                         history_service) {}

FeedV2Builder::~FeedV2Builder() = default;

void FeedV2Builder::AddListener(
    mojo::PendingRemote<mojom::FeedListener> listener) {
  auto id = listeners_.Add(std::move(listener));

  auto* instance = listeners_.Get(id);
  CHECK(instance);

  instance->OnUpdateAvailable(hash_);
}

void FeedV2Builder::BuildFollowingFeed(
    const BraveNewsSubscriptions& subscriptions,
    BuildFeedCallback callback) {
  FeedItems raw_feed_items;
  base::ranges::transform(raw_feed_items_, std::back_inserter(raw_feed_items),
                          [](const auto& item) { return item->Clone(); });
  GenerateFeed(
      subscriptions, {.signals = true},
      mojom::FeedV2Type::NewFollowing(mojom::FeedV2FollowingType::New()),
      base::BindOnce([](FeedGenerationInfo info) {
        return GenerateBasicFeed(std::move(info),
                                 base::BindRepeating(&PickRoulette),
                                 base::BindRepeating(&PickRoulette));
      }),
      std::move(callback));
}

void FeedV2Builder::BuildChannelFeed(
    const BraveNewsSubscriptions& subscriptions,
    const std::string& channel,
    BuildFeedCallback callback) {
  FeedItems raw_feed_items;
  base::ranges::transform(raw_feed_items_, std::back_inserter(raw_feed_items),
                          [](const auto& item) { return item->Clone(); });
  GenerateFeed(
      subscriptions, {.signals = true},
      mojom::FeedV2Type::NewChannel(mojom::FeedV2ChannelType::New(channel)),
      base::BindOnce(
          [](std::string channel, FeedGenerationInfo info) {
            FeedItems feed_items;
            for (const auto& item : info.feed_items) {
              if (!item->is_article()) {
                continue;
              }

              auto publisher_it =
                  info.publishers.find(item->get_article()->data->publisher_id);
              if (publisher_it == info.publishers.end()) {
                continue;
              }

              const auto& locale = info.locale;

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

              feed_items.push_back(item->Clone());
            }

            info.feed_items = std::move(feed_items);

            return GenerateBasicFeed(std::move(info),
                                     base::BindRepeating(&PickRoulette),
                                     base::BindRepeating(&PickRoulette));
          },
          channel),
      std::move(callback));
}

void FeedV2Builder::BuildPublisherFeed(
    const BraveNewsSubscriptions& subscriptions,
    const std::string& publisher_id,
    BuildFeedCallback callback) {
  GenerateFeed(
      subscriptions, {.signals = true},
      mojom::FeedV2Type::NewPublisher(
          mojom::FeedV2PublisherType::New(publisher_id)),
      base::BindOnce(
          [](const std::string& publisher_id, FeedGenerationInfo info) {
            FeedItems items;

            for (const auto& item : info.feed_items) {
              if (!item->is_article()) {
                continue;
              }
              if (item->get_article()->data->publisher_id != publisher_id) {
                continue;
              }

              items.push_back(item->Clone());
            }

            // Sort by publish time.
            base::ranges::sort(items, [](const auto& a, const auto& b) {
              return a->get_article()->data->publish_time >
                     b->get_article()->data->publish_time;
            });

            info.feed_items = std::move(items);

            return GenerateBasicFeed(std::move(info),
                                     base::BindRepeating(&PickFirstIndex),
                                     base::BindRepeating(&PickFirstIndex));
          },
          publisher_id),
      std::move(callback));
}

void FeedV2Builder::BuildAllFeed(const BraveNewsSubscriptions& subscriptions,
                                 BuildFeedCallback callback) {
  GenerateFeed(subscriptions, {.signals = true, .suggested_publishers = true},
               mojom::FeedV2Type::NewAll(mojom::FeedV2AllType::New()),
               base::BindOnce(&GenerateAllFeed), std::move(callback));
}

void FeedV2Builder::EnsureFeedIsUpdating(
    const BraveNewsSubscriptions& subscriptions) {
  UpdateData(subscriptions,
             {.signals = true,
              .suggested_publishers = true,
              .feed = true,
              .topics = true},
             base::DoNothing());
}

void FeedV2Builder::GetSignals(const BraveNewsSubscriptions& subscriptions,
                               GetSignalsCallback callback) {
  UpdateData(subscriptions, {.signals = true},
             base::BindOnce(
                 [](base::WeakPtr<FeedV2Builder> builder,
                    GetSignalsCallback callback) {
                   if (!builder) {
                     return;
                   }
                   base::flat_map<std::string, Signal> signals;
                   for (const auto& [key, value] : builder->signals_) {
                     signals[key] = value->Clone();
                   }
                   std::move(callback).Run(std::move(signals));
                 },
                 weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void FeedV2Builder::RecheckFeedHash(
    const BraveNewsSubscriptions& subscriptions) {
  const auto& publishers = publishers_controller_->GetLastPublishers();
  auto channels = channels_controller_->GetChannelsFromPublishers(
      publishers, subscriptions);
  std::tie(hash_, subscribed_count_) =
      GetFeedHashAndSubscribedCount(channels, publishers, feed_etags_);
  for (const auto& listener : listeners_) {
    listener->OnUpdateAvailable(hash_);
  }
}

void FeedV2Builder::UpdateData(const BraveNewsSubscriptions& subscriptions,
                               UpdateSettings settings,
                               UpdateCallback callback) {
  if (current_update_) {
    if (current_update_->IsSufficient(settings)) {
      current_update_->callbacks.push_back(std::move(callback));
    } else {
      if (!next_update_) {
        next_update_.emplace(subscriptions, std::move(settings),
                             std::move(callback));
      } else {
        // Use the most recent subscription data we have.
        next_update_->subscriptions = subscriptions;
        next_update_->AlsoUpdate(std::move(settings), std::move(callback));
      }
    }
    return;
  }

  current_update_.emplace(subscriptions, std::move(settings),
                          std::move(callback));

  PrepareAndFetch();
}

void FeedV2Builder::PrepareAndFetch() {
  DVLOG(1) << __FUNCTION__;
  CHECK(current_update_);

  if (current_update_->settings.signals) {
    signals_.clear();
  }

  if (current_update_->settings.feed) {
    raw_feed_items_.clear();
  }

  if (current_update_->settings.suggested_publishers) {
    suggested_publisher_ids_.clear();
  }

  if (current_update_->settings.topics) {
    topics_.clear();
  }

  FetchFeed();
}

void FeedV2Builder::FetchFeed() {
  DVLOG(1) << __FUNCTION__;
  CHECK(current_update_);

  // Don't refetch the feed if we have items (clearing the items will trigger a
  // refresh).
  if (!raw_feed_items_.empty()) {
    // Note: This isn't ideal because we double move the raw_feed_items and
    // etags, but it makes the algorithm easier to follow.
    OnFetchedFeed(std::move(raw_feed_items_), std::move(feed_etags_));
    return;
  }

  fetcher_.FetchFeed(
      current_update_->subscriptions,
      base::BindOnce(&FeedV2Builder::OnFetchedFeed,
                     // Unretained is safe here because the FeedFetcher is owned
                     // by this and uses weak pointers internally.
                     base::Unretained(this)));
}

void FeedV2Builder::OnFetchedFeed(FeedItems items, ETags tags) {
  DVLOG(1) << __FUNCTION__;

  raw_feed_items_ = std::move(items);
  feed_etags_ = std::move(tags);

  CalculateSignals();
}

void FeedV2Builder::CalculateSignals() {
  DVLOG(1) << __FUNCTION__;
  CHECK(current_update_);

  // Don't recalculate the signals unless they've been cleared.
  if (!signals_.empty()) {
    // Note: We double move signals here because it makes the algorithm easier
    // to follow.
    OnCalculatedSignals(std::move(signals_));
    return;
  }

  signal_calculator_.GetSignals(
      current_update_->subscriptions, raw_feed_items_,
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
  CHECK(current_update_);

  // Don't get suggested publisher ids unless they're empty - clearing indicates
  // we should refetch.
  if (!suggested_publisher_ids_.empty()) {
    OnGotSuggestedPublisherIds(std::move(suggested_publisher_ids_));
    return;
  }
  suggestions_controller_->GetSuggestedPublisherIds(
      current_update_->subscriptions,
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

  // Don't refetch topics, unless we need to.
  if (!topics_.empty()) {
    OnGotTopics(std::move(topics_));
    return;
  }

  topics_fetcher_.GetTopics(publishers_controller_->GetLastLocale(),
                            base::BindOnce(&FeedV2Builder::OnGotTopics,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void FeedV2Builder::OnGotTopics(TopicsResult topics) {
  DVLOG(1) << __FUNCTION__ << " (topic count: " << topics.size() << ")";
  topics_ = std::move(topics);

  NotifyUpdateCompleted();
}

void FeedV2Builder::NotifyUpdateCompleted() {
  CHECK(current_update_);

  // Recalculate the hash_ - this will be used to mark the source of generated
  // feeds.
  const auto& publishers = publishers_controller_->GetLastPublishers();
  auto channels = channels_controller_->GetChannelsFromPublishers(
      publishers, current_update_->subscriptions);
  std::tie(hash_, subscribed_count_) =
      GetFeedHashAndSubscribedCount(channels, publishers, feed_etags_);

  for (auto& callback : current_update_->callbacks) {
    std::move(callback).Run();
  }

  // Notify listeners of updated hash.
  for (const auto& listener : listeners_) {
    listener->OnUpdateAvailable(hash_);
  }

  // Move |next_update_| into |current_update_|, and kick it off (if set).
  current_update_ = std::move(next_update_);
  next_update_ = std::nullopt;

  if (current_update_) {
    PrepareAndFetch();
  }
}

void FeedV2Builder::GenerateFeed(const BraveNewsSubscriptions& subscriptions,
                                 UpdateSettings settings,
                                 mojom::FeedV2TypePtr type,
                                 FeedGenerator generator,
                                 BuildFeedCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  UpdateData(
      subscriptions, std::move(settings),
      base::BindOnce(
          [](const base::WeakPtr<FeedV2Builder> builder,
             mojom::FeedV2TypePtr type, FeedGenerator generate,
             BuildFeedCallback callback) {
            if (!builder) {
              std::move(callback).Run(mojom::FeedV2::New());
              return;
            }

            const auto& publishers =
                builder->publishers_controller_->GetLastPublishers();
            const auto& locale =
                builder->publishers_controller_->GetLastLocale();
            std::vector<std::string> channels;
            for (const auto& [channel_id, channel] :
                 builder->channels_controller_->GetChannelsFromPublishers(
                     publishers, builder->current_update_->subscriptions)) {
              if (base::Contains(channel->subscribed_locales, locale)) {
                channels.push_back(channel_id);
              }
            }

            FeedGenerationInfo info(
                locale, builder->raw_feed_items_, publishers,
                std::move(channels), builder->signals_,
                builder->suggested_publisher_ids_, builder->topics_);

            // We post this to another thread, because the generation process
            // can be quite slow. It's safe because all data |generator|
            // requires is copied into the lambda.
            base::ThreadPool::PostTaskAndReplyWithResult(
                FROM_HERE, base::BindOnce(std::move(generate), std::move(info)),
                base::BindOnce(
                    [](size_t raw_feed_items_size, size_t subscribed_count,
                       bool has_publishers, std::string hash,
                       mojom::FeedV2TypePtr type, BuildFeedCallback callback,
                       mojom::FeedV2Ptr feed) {
                      DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
                      feed->construct_time = base::Time::Now();
                      feed->type = std::move(type);
                      feed->source_hash = hash;

                      if (feed->items.empty()) {
                        // If we have no subscribed items and we've loaded
                        // the list of publishers (which we might not have,
                        // if we're offline) then we're not subscribed to
                        // any feeds.
                        if (subscribed_count == 0 && has_publishers) {
                          feed->error = mojom::FeedV2Error::NoFeeds;
                          // If we don't have any raw feed items (and we're
                          // subscribed to some feeds) then fetching must
                          // have failed.
                        } else if (raw_feed_items_size == 0) {
                          feed->error = mojom::FeedV2Error::ConnectionError;
                          // Otherwise, this feed must have no articles.
                        } else {
                          feed->error = mojom::FeedV2Error::NoArticles;
                        }
                      }

                      std::move(callback).Run(std::move(feed));
                    },
                    builder->raw_feed_items_.size(), builder->subscribed_count_,
                    !publishers.empty(), builder->hash_, std::move(type),
                    std::move(callback)));
          },
          weak_ptr_factory_.GetWeakPtr(), std::move(type), std::move(generator),
          std::move(callback)));
}

}  // namespace brave_news
