// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/brave_news/browser/feed_v2_builder.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/rand_util.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_news/api/topics.h"
#include "brave/components/brave_news/browser/background_history_querier.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/feed_generation_info.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/browser/peeking_card.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/signal_calculator.h"
#include "brave/components/brave_news/browser/topics_fetcher.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_news {

namespace {

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

// Generates a standard block:
// 1. Hero Article
// 2. 1 - 5 Inline Articles (a percentage of which might be discover cards).
std::vector<mojom::FeedItemV2Ptr> GenerateBlock(FeedGenerationInfo& info,
                                                PickArticles hero_picker,
                                                PickArticles article_picker,
                                                double inline_discovery_ratio) {
  DVLOG(1) << __FUNCTION__;
  std::vector<mojom::FeedItemV2Ptr> result;
  if (info.GetArticleInfos().empty()) {
    return result;
  }

  auto hero_article = info.PickAndConsume(hero_picker);

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
      // Picking a discovery article works the same way as as a normal roulette
      // selection, but we only consider articles that:
      // 1. The user hasn't subscribed to.
      // 2. **AND** The user hasn't visited.
      generated = info.PickAndConsume(
          base::BindRepeating([](const ArticleInfos& articles) {
            return PickRouletteWithWeighting(
                articles,
                base::BindRepeating([](const mojom::FeedItemMetadataPtr& data,
                                       const ArticleMetadata& meta) {
                  if (!meta.discoverable) {
                    return 0.0;
                  }

                  if (meta.subscribed) {
                    return 0.0;
                  }
                  return meta.pop_recency;
                }));
          }));
    } else {
      generated = info.PickAndConsume(article_picker);
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

// Generates a block from sampled content groups:
// 1. Hero Article
// 2. 1 - 5 Inline Articles (a percentage of which might be discover cards).
std::vector<mojom::FeedItemV2Ptr> GenerateBlockFromContentGroups(
    FeedGenerationInfo& info,
    PickArticles pick_hero = base::NullCallback()) {
  DVLOG(1) << __FUNCTION__;
  // Ratio of inline articles to discovery articles.
  // discover ratio % of the time, we should do a discover card here instead
  // of a roulette card.
  // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.4rkb0vecgekl
  const double inline_discovery_ratio =
      features::kBraveNewsInlineDiscoveryRatio.Get();

  auto eligible_content_groups = info.GetEligibleContentGroups();
  std::vector<mojom::FeedItemV2Ptr> result;
  if (info.GetArticleInfos().empty() || eligible_content_groups.empty()) {
    LOG(ERROR) << "Finished feed generation (no eligible content groups or no "
                  "articles)";
    return result;
  }

  base::flat_map<std::string, std::vector<std::string>>
      publisher_id_to_channels;
  for (const auto& [publisher_id, publisher] : info.publishers()) {
    publisher_id_to_channels[publisher_id] =
        GetChannelsForPublisher(info.locale(), publisher);
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
               const mojom::FeedItemMetadataPtr& article,
               const ArticleMetadata& meta) {
              if (is_hero) {
                auto image_url = article->image->is_padded_image_url()
                                     ? article->image->get_padded_image_url()
                                     : article->image->get_image_url();
                if (!image_url.is_valid()) {
                  return 0.0;
                }
              }

              if (/*is_channel*/ content_group.second) {
                auto channels =
                    publisher_id_to_channels.find(article->publisher_id);
                if (base::Contains(channels->second, content_group.first)) {
                  return meta.weighting;
                }

                return 0.0;
              }

              return article->publisher_id == content_group.first
                         ? meta.weighting
                         : 0.0;
            },
            is_hero, SampleContentGroup(eligible_content_groups),
            publisher_id_to_channels, locale);
      },
      info.locale(), std::move(eligible_content_groups),
      std::move(publisher_id_to_channels));

  if (pick_hero.is_null()) {
    pick_hero = base::BindRepeating(
        [](GetWeighting weighting, const ArticleInfos& articles) {
          return PickRouletteWithWeighting(articles, std::move(weighting));
        },
        get_weighting.Run(true));
  }

  PickArticles pick_article = base::BindRepeating(
      [](base::RepeatingCallback<GetWeighting(bool)> gen_weighting,
         const ArticleInfos& articles) {
        return PickRouletteWithWeighting(articles, gen_weighting.Run(false));
      },
      std::move(get_weighting));

  return GenerateBlock(info, pick_hero, pick_article, inline_discovery_ratio);
}

// Generates a Channel Block
// 1 Hero Articles (from the channel)
// 1 - 5 Inline Articles (from the channel)
// This function is the same as GenerateBlock, except that the available
// articles are filtered to only be from the specified channel.
// https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.kxe6xeqm2vfn
std::vector<mojom::FeedItemV2Ptr> GenerateChannelBlock(
    FeedGenerationInfo& info,
    const std::string& channel) {
  DVLOG(1) << __FUNCTION__;

  auto channel_picker = base::BindRepeating(&PickChannelRoulette, channel);
  auto block = GenerateBlock(info, channel_picker, channel_picker, 0);

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
    FeedGenerationInfo& info) {
  auto topics = info.topics();
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

    auto item = FromTopicArticle(info.publishers(), articles.at(0));
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
    FeedGenerationInfo& feed_generation_info) {
  if (feed_generation_info.topics().empty()) {
    return {};
  }
  DVLOG(1) << __FUNCTION__;
  auto result = mojom::Cluster::New();
  result->type = mojom::ClusterType::TOPIC;

  auto& [topic, articles] = feed_generation_info.topics().front();
  result->id = topic.claude_title_short;

  uint64_t max_articles = features::kBraveNewsMaxBlockCards.Get();
  for (const auto& article : articles) {
    auto item = FromTopicArticle(feed_generation_info.publishers(), article);
    result->articles.push_back(mojom::ArticleElements::NewArticle(
        mojom::Article::New(std::move(item), false)));

    // For now, we truncate at |max_articles|. In future we may want to include
    // more articles here and have the option to show more in the front end.
    if (result->articles.size() >= max_articles) {
      break;
    }
  }

  // Make sure we don't reuse the topic by excluding it from our span.
  feed_generation_info.topics() =
      base::span(feed_generation_info.topics()).subspan(1u);

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
    FeedGenerationInfo& feed_generation_info) {
  const auto channels = feed_generation_info.EligibleChannels();
  // If we have no channels, and no topics there's nothing we can do here.
  if (channels.empty() && feed_generation_info.topics().empty()) {
    DVLOG(1) << "Nothing (no subscribed channels or unshown topics)";
    return {};
  }

  // Determine whether we should generate a channel or topic cluster.
  auto generate_channel =
      (!channels.empty() &&
       base::RandDouble() < features::kBraveNewsCategoryTopicRatio.Get()) ||
      feed_generation_info.topics().empty();

  if (generate_channel) {
    auto channel = PickRandom(channels);
    DVLOG(1) << "Cluster Block (channel: " << channel << ")";
    return GenerateChannelBlock(feed_generation_info, PickRandom(channels));
  } else {
    DVLOG(1) << "Cluster Block (topic)";
    return GenerateTopicBlock(feed_generation_info);
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
    FeedGenerationInfo& info) {
  DVLOG(1) << __FUNCTION__;

  auto suggested_publisher_ids = info.suggested_publisher_ids();
  std::vector<mojom::FeedItemV2Ptr> result;
  if (!suggested_publisher_ids.empty()) {
    size_t preferred_count = 3;
    size_t count = std::min(preferred_count, suggested_publisher_ids.size());
    std::vector<std::string> suggestions(
        suggested_publisher_ids.begin(),
        suggested_publisher_ids.begin() + count);

    info.suggested_publisher_ids() =
        base::span(suggested_publisher_ids).subspan(count);

    DVLOG(1) << "Generating publisher suggestions (discover)";
    result.push_back(mojom::FeedItemV2::NewDiscover(
        mojom::Discover::New(std::move(suggestions))));
  }

  return result;
}

}  // namespace

FeedV2Builder::UpdateRequest::UpdateRequest(SubscriptionsSnapshot subscriptions,
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
                                                  PickArticles pick_article,
                                                  PickArticles pick_peeking) {
  DVLOG(1) << __FUNCTION__;
  auto feed = mojom::FeedV2::New();

  constexpr size_t kIterationsPerAd = 2;
  size_t blocks = 0;
  while (!info.GetArticleInfos().empty()) {
    auto items = GenerateBlock(
        info, feed->items.empty() ? pick_peeking : pick_hero, pick_article,
        /*inline_discovery_ratio=*/0);
    if (items.empty()) {
      break;
    }

    // After the first block, every second block should be an ad.
    if (blocks % kIterationsPerAd == 0 && blocks != 0) {
      std::ranges::move(GenerateSpecialBlock(info), std::back_inserter(items));
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
  auto feed = mojom::FeedV2::New();

  auto add_items = [&feed](std::vector<mojom::FeedItemV2Ptr>& items) {
    base::ranges::move(items, std::back_inserter(feed->items));
  };

  // If we aren't subscribed to anything, or we failed to fetch any articles
  // from the internet, don't try and generate a feed.
  if (info.GetEligibleContentGroups().size() == 0 ||
      info.raw_feed_items().size() == 0) {
    return feed;
  }

  // Step 1: Generate the initial block. We have a special algorithm for the
  // first hero card
  // (https://docs.google.com/document/d/1HH7pohTPp-8uqdccwK4phOrwqK0-NRo-kkBxTDuWUsM)
  // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.rkq699fwps0
  std::vector<mojom::FeedItemV2Ptr> initial_block =
      GenerateBlockFromContentGroups(
          info, base::BindRepeating(&PickPeekingCard, info.subscriptions(),
                                    GetTopStoryUrls(info.topics())));
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
  auto top_news_block = GenerateTopTopicsBlock(info);
  if (top_news_block.empty()) {
    top_news_block = GenerateChannelBlock(info, kTopNewsChannel);
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
      items = GenerateBlockFromContentGroups(info);
    } else if (iteration_type == 1) {
      // Step 5: Block or Cluster Generation
      // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.tpvsjkq0lzmy
      // Half the time, a normal block
      if (TossCoin()) {
        DVLOG(1) << "Step 5: Standard Block";
        items = GenerateBlockFromContentGroups(info);
      } else {
        items = GenerateClusterBlock(info);
      }
    } else if (iteration_type == 2) {
      // Step 6: Optional special card or Advertisement
      // https://docs.google.com/document/d/1bSVHunwmcHwyQTpa3ab4KRbGbgNQ3ym_GHvONnrBypg/edit#heading=h.n1ipt86esc34
      if (TossCoin()) {
        DVLOG(1) << "Step 6.1: Special Block";
        items = GenerateSpecialBlock(info);
      } else {
        DVLOG(1) << "Step 6.2: Advertisement";
        items = GenerateAd();
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
    BackgroundHistoryQuerier& history_querier,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : publishers_controller_(publishers_controller),
      channels_controller_(channels_controller),
      suggestions_controller_(suggestions_controller),
      fetcher_(publishers_controller, url_loader_factory),
      topics_fetcher_(url_loader_factory),
      signal_calculator_(publishers_controller,
                         channels_controller,
                         history_querier) {}

FeedV2Builder::~FeedV2Builder() = default;

void FeedV2Builder::AddListener(
    mojo::PendingRemote<mojom::FeedListener> listener) {
  auto id = listeners_.Add(std::move(listener));

  auto* instance = listeners_.Get(id);
  CHECK(instance);

  instance->OnUpdateAvailable(hash_);
}

void FeedV2Builder::BuildFollowingFeed(
    const SubscriptionsSnapshot& subscriptions,
    BuildFeedCallback callback) {
  FeedItems raw_feed_items;
  base::ranges::transform(raw_feed_items_, std::back_inserter(raw_feed_items),
                          [](const auto& item) { return item->Clone(); });
  GenerateFeed(
      subscriptions, {.signals = true},
      mojom::FeedV2Type::NewFollowing(mojom::FeedV2FollowingType::New()),
      base::BindOnce([](FeedGenerationInfo info) {
        auto subscriptions = info.subscriptions();
        auto top_stories = GetTopStoryUrls(info.topics());
        return GenerateBasicFeed(
            std::move(info), base::BindRepeating(&PickRoulette),
            base::BindRepeating(&PickRoulette),
            base::BindRepeating(&PickPeekingCard, std::move(subscriptions),
                                std::move(top_stories)));
      }),
      std::move(callback));
}

void FeedV2Builder::BuildChannelFeed(const SubscriptionsSnapshot& subscriptions,
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
            for (const auto& item : info.raw_feed_items()) {
              if (!item->is_article()) {
                continue;
              }

              auto publisher_it = info.publishers().find(
                  item->get_article()->data->publisher_id);
              if (publisher_it == info.publishers().end()) {
                continue;
              }

              const auto& locale = info.locale();

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

            info.raw_feed_items() = std::move(feed_items);
            auto subscriptions = info.subscriptions();
            auto top_stories = GetTopStoryUrls(info.topics());
            return GenerateBasicFeed(
                std::move(info), base::BindRepeating(&PickRoulette),
                base::BindRepeating(&PickRoulette),
                base::BindRepeating(&PickPeekingCard, std::move(subscriptions),
                                    std::move(top_stories)));
          },
          channel),
      std::move(callback));
}

void FeedV2Builder::BuildPublisherFeed(
    const SubscriptionsSnapshot& subscriptions,
    const std::string& publisher_id,
    BuildFeedCallback callback) {
  GenerateFeed(
      subscriptions, {.signals = true},
      mojom::FeedV2Type::NewPublisher(
          mojom::FeedV2PublisherType::New(publisher_id)),
      base::BindOnce(
          [](const std::string& publisher_id, FeedGenerationInfo info) {
            FeedItems items;

            for (const auto& item : info.raw_feed_items()) {
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

            // Override the raw feed items.
            info.raw_feed_items() = std::move(items);

            return GenerateBasicFeed(std::move(info),
                                     base::BindRepeating(&PickFirstIndex),
                                     base::BindRepeating(&PickFirstIndex),
                                     base::BindRepeating(&PickFirstIndex));
          },
          publisher_id),
      std::move(callback));
}

void FeedV2Builder::BuildAllFeed(const SubscriptionsSnapshot& subscriptions,
                                 BuildFeedCallback callback) {
  GenerateFeed(subscriptions, {.signals = true, .suggested_publishers = true},
               mojom::FeedV2Type::NewAll(mojom::FeedV2AllType::New()),
               base::BindOnce(&GenerateAllFeed), std::move(callback));
}

void FeedV2Builder::GetSignals(const SubscriptionsSnapshot& subscriptions,
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

void FeedV2Builder::GetLatestHash(const SubscriptionsSnapshot& subscriptions,
                                  bool refetch_data,
                                  HashCallback callback) {
  UpdateData(
      subscriptions,
      {.signals = true,
       .suggested_publishers = true,
       .feed = refetch_data,
       .topics = refetch_data},
      base::BindOnce(
          [](base::WeakPtr<FeedV2Builder> builder,
             const SubscriptionsSnapshot& subscriptions,
             HashCallback callback) {
            if (!builder) {
              return;
            }
            const auto& publishers =
                builder->publishers_controller_->last_publishers();
            auto channels =
                builder->channels_controller_->GetChannelsFromPublishers(
                    publishers, subscriptions);
            std::tie(builder->hash_, builder->subscribed_count_) =
                GetFeedHashAndSubscribedCount(channels, publishers,
                                              builder->feed_etags_);
            std::move(callback).Run(builder->hash_);
          },
          weak_ptr_factory_.GetWeakPtr(), subscriptions, std::move(callback)));
}

void FeedV2Builder::UpdateData(const SubscriptionsSnapshot& subscriptions,
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
  const auto& publishers = publishers_controller_->last_publishers();
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

void FeedV2Builder::GenerateFeed(const SubscriptionsSnapshot& subscriptions,
                                 UpdateSettings settings,
                                 mojom::FeedV2TypePtr type,
                                 FeedGenerator generator,
                                 BuildFeedCallback callback) {
  UpdateData(
      subscriptions, std::move(settings),
      base::BindOnce(
          [](const base::WeakPtr<FeedV2Builder> builder,
             const SubscriptionsSnapshot& subscriptions,
             mojom::FeedV2TypePtr type, FeedGenerator generate,
             BuildFeedCallback callback) {
            if (!builder) {
              std::move(callback).Run(mojom::FeedV2::New());
              return;
            }

            const auto& publishers =
                builder->publishers_controller_->last_publishers();
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
                subscriptions, locale, builder->raw_feed_items_, publishers,
                std::move(channels), builder->signals_,
                builder->suggested_publisher_ids_, builder->topics_);

            auto feed = std::move(generate).Run(std::move(info));
            feed->construct_time = base::Time::Now();
            feed->type = std::move(type);
            feed->source_hash = builder->hash_;

            if (feed->items.empty()) {
              // If we have no subscribed items and we've loaded
              // the list of publishers (which we might not have,
              // if we're offline) then we're not subscribed to
              // any feeds.
              if (builder->subscribed_count_ == 0 && !publishers.empty()) {
                feed->error = mojom::FeedV2Error::NoFeeds;
                // If we don't have any raw feed items (and we're
                // subscribed to some feeds) then fetching must
                // have failed.
              } else if (builder->raw_feed_items_.size() == 0) {
                feed->error = mojom::FeedV2Error::ConnectionError;
                // Otherwise, this feed must have no articles.
              } else {
                feed->error = mojom::FeedV2Error::NoArticles;
              }
            }

            std::move(callback).Run(std::move(feed));
          },
          weak_ptr_factory_.GetWeakPtr(), subscriptions, std::move(type),
          std::move(generator), std::move(callback)));
}

}  // namespace brave_news
