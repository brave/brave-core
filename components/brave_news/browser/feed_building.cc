// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_building.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom-shared.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/features.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace brave_news {

namespace {

using mojom::CardType;

// This controls the order to display "card" and content types on every
// platform. Each "page" of content is a repeat of
// `page_content_order + random_content_order`
std::vector<CardType> g_page_content_order = {
    CardType::HEADLINE,        CardType::HEADLINE,
    CardType::HEADLINE_PAIRED, CardType::PROMOTED_ARTICLE,
    CardType::CATEGORY_GROUP,  CardType::HEADLINE,
    CardType::HEADLINE,        CardType::HEADLINE_PAIRED,
    CardType::HEADLINE_PAIRED, CardType::DISPLAY_AD,
    CardType::HEADLINE,        CardType::HEADLINE,
    CardType::PUBLISHER_GROUP, CardType::HEADLINE_PAIRED,
    CardType::HEADLINE,        CardType::DEALS};

std::vector<CardType> g_random_content_order = {CardType::HEADLINE,
                                                CardType::HEADLINE_PAIRED};

mojom::FeedItemPtr FromArticle(mojom::ArticlePtr article) {
  return mojom::FeedItem::NewArticle(std::move(article));
}

mojom::FeedItemPtr FromDeal(mojom::DealPtr deal) {
  return mojom::FeedItem::NewDeal(std::move(deal));
}

mojom::FeedItemPtr FromPromotedArticle(mojom::PromotedArticlePtr item) {
  return mojom::FeedItem::NewPromotedArticle(std::move(item));
}

bool MatchesDealsCategory(const std::string& category_name, mojom::Deal* deal) {
  return (deal->offers_category == category_name);
}

// Removes {count} items from a list and places them in a vector,
// optionally filtering via {predicate}. Wraps item in a FeedItem
// which, since it is a union type, must be created via {create}.
template <class T>
bool Take(
    size_t count,
    std::list<mojo::StructPtr<T>>* articles,
    std::vector<mojom::FeedItemPtr>* results,
    base::RepeatingCallback<mojom::FeedItemPtr(mojo::StructPtr<T>)> create =
        base::BindRepeating(&FromArticle),
    base::RepeatingCallback<bool(T*)> predicate = base::BindRepeating([](T* a) {
      return true;
    })) {
  auto it = articles->begin();
  while (it != articles->end() && results->size() < count) {
    auto should_take = predicate.Run(it->get());
    if (should_take) {
      mojom::FeedItemPtr item = create.Run(*std::make_move_iterator(it));
      results->emplace_back(std::move(item));
      it = articles->erase(it);
    } else {
      it++;
    }
  }
  return (results->size() == count);
}

// Like Take<T> except selects randomly instead of in order.
template <class T>
void TakeRandom(
    size_t count,
    std::list<mojo::StructPtr<T>>* articles,
    std::vector<mojom::FeedItemPtr>* results,
    base::RepeatingCallback<mojom::FeedItemPtr(mojo::StructPtr<T>)> create =
        FromArticle,
    base::RepeatingCallback<bool(T*)> predicate = {[](T* a) { return true; }}) {
  auto it = articles->begin();
  std::vector<typename std::list<mojo::StructPtr<T>>::iterator>
      matching_iterators;
  while (it != articles->end()) {
    auto should_take = predicate.Run(it->get());
    if (should_take) {
      matching_iterators.push_back(it);
    }
    it++;
  }

  base::RandomShuffle(matching_iterators.begin(), matching_iterators.end());
  if (matching_iterators.size() > count) {
    matching_iterators.resize(count);
  }
  for (auto& matching_iterator : matching_iterators) {
    auto a = std::make_move_iterator(matching_iterator);
    auto item = create.Run(*a);
    results->emplace_back(std::move(item));
    articles->erase(matching_iterator);
  }
}

// Decides which content to take for a specific item in the feed.
// Items approximately correspond to "cards" in the UI, although an item
// could be 2 cards (e.g. HEADLINE_PAIRED) or multiple
// articles (e.g. CATEGORY_GROUP).
void BuildFeedPageItem(std::list<mojom::ArticlePtr>* articles,
                       std::list<mojom::PromotedArticlePtr>* promoted_articles,
                       std::list<mojom::DealPtr>* deals,
                       const std::string& deal_category_name,
                       const std::string& article_category_name,
                       bool is_random,
                       mojom::FeedPageItemPtr* page_item_ptr) {
  auto* page_item = page_item_ptr->get();
  if (is_random) {
    // Additional difference for is_random is that we only consider items from
    // the last 48hrs.
    base::Time time_limit = base::Time::Now() - base::Days(2);
    auto match_is_recent = base::BindRepeating(
        [](base::Time time_limit, mojom::Article* article) {
          return (article->data->publish_time >= time_limit);
        },
        time_limit);
    switch (page_item->card_type) {
      case CardType::HEADLINE:
        TakeRandom<mojom::Article>(1u, articles, &page_item->items,
                                   base::BindRepeating(&FromArticle),
                                   match_is_recent);
        break;
      case CardType::HEADLINE_PAIRED:
        TakeRandom<mojom::Article>(2u, articles, &page_item->items,
                                   base::BindRepeating(&FromArticle),
                                   match_is_recent);
        break;
      default:
        VLOG(1) << "Card Type not handled for is_random: "
                << page_item->card_type;
        break;
    }
  }
  // Not having enough articles is the only real reason to abandon a page.
  switch (page_item->card_type) {
    case CardType::HEADLINE:
      Take<mojom::Article>(1u, articles, &page_item->items,
                           base::BindRepeating(&FromArticle));
      break;
    case CardType::HEADLINE_PAIRED:
      Take<mojom::Article>(2u, articles, &page_item->items,
                           base::BindRepeating(&FromArticle));
      break;
    case CardType::CATEGORY_GROUP:
      Take<mojom::Article>(
          3u, articles, &page_item->items, base::BindRepeating(&FromArticle),
          base::BindRepeating(
              [](const std::string& article_category_name,
                 mojom::Article* article) {
                return (article->data->category_name == article_category_name);
              },
              article_category_name));
      break;
    case CardType::PUBLISHER_GROUP: {
      // Choose the first publisher available
      std::string publisher_id;
      for (auto& article : *articles) {
        if (article->data->publisher_id.empty()) {
          continue;
        }
        publisher_id = article.get()->data->publisher_id;
        break;
      }
      Take<mojom::Article>(
          3u, articles, &page_item->items, base::BindRepeating(&FromArticle),
          base::BindRepeating(
              [](const std::string& publisher_id, mojom::Article* article) {
                return (article->data->publisher_id == publisher_id);
              },
              publisher_id));
      break;
    }
    case CardType::DEALS:
      Take<mojom::Deal>(
          3u, deals, &page_item->items, base::BindRepeating(FromDeal),
          base::BindRepeating(
              [](const std::string& deal_category_name, mojom::Deal* deal) {
                return MatchesDealsCategory(deal_category_name, deal);
              },
              deal_category_name));
      if (page_item->items.size() < 3u) {
        // Supplement with deals from other categories
        size_t supplemental_count =
            std::min(3u - page_item->items.size(), deals->size());
        Take<mojom::Deal>(supplemental_count, deals, &page_item->items,
                          base::BindRepeating(&FromDeal));
      }
      break;
    case CardType::DISPLAY_AD:
      // Content is retrieved by front-end at a time
      // closer to this item being viewed.
      break;
    case CardType::PROMOTED_ARTICLE:
      Take<mojom::PromotedArticle>(1u, promoted_articles, &page_item->items,
                                   base::BindRepeating(&FromPromotedArticle));
      break;
  }
}

mojom::FeedItemMetadataPtr& MetadataFromFeedItem(
    const mojom::FeedItemPtr& item) {
  switch (item->which()) {
    case mojom::FeedItem::Tag::kArticle:
      return item->get_article()->data;
    case mojom::FeedItem::Tag::kDeal:
      return item->get_deal()->data;
    case mojom::FeedItem::Tag::kPromotedArticle:
      return item->get_promoted_article()->data;
  }
}

}  // namespace

bool ShouldDisplayFeedItem(const mojom::FeedItemPtr& feed_item,
                           const Publishers* publishers,
                           const Channels& channels) {
  // Filter out articles from publishers we're ignoring
  const auto& data = MetadataFromFeedItem(feed_item);
  if (!publishers->contains(data->publisher_id)) {
    VLOG(1) << "Found article with unknown publisher_id. PublisherId: "
            << data->publisher_id;
    return false;
  }
  const auto& publisher = publishers->at(data->publisher_id);
  if (publisher->user_enabled_status ==
      brave_news::mojom::UserEnabled::DISABLED) {
    VLOG(1) << "Hiding article for disabled-by-user publisher "
            << data->publisher_id << ": " << publisher->publisher_name;
    return false;
  }

  // Direct publishers should be shown, even though they aren't in any locales,
  // and their enabled status is |NOT_MODIFIED|.
  if (publisher->type == brave_news::mojom::PublisherType::DIRECT_SOURCE) {
    VLOG(2) << "Showing article for direct feed " << data->publisher_id << ": "
            << publisher->publisher_name
            << " because direct feeds are always shown.";
    return true;
  }

  if (publisher->user_enabled_status ==
      brave_news::mojom::UserEnabled::NOT_MODIFIED) {
    // If the publisher is NOT_MODIFIED then display it if any of the channels
    // it belongs to are subscribed to.
    for (const auto& locale_info : publisher->locales) {
      for (const auto& channel_id : locale_info->channels) {
        if (channels.contains(channel_id)) {
          const auto& channel = channels.at(channel_id);
          if (base::Contains(channel->subscribed_locales,
                             locale_info->locale)) {
            VLOG(2) << "Showing article because publisher "
                    << data->publisher_id << ": " << publisher->publisher_name
                    << " is in channel " << locale_info->locale << "."
                    << channel_id << " which is subscribed to.";
            return true;
          }
        }
      }
    }

    // The publisher isn't in a subscribed channel, and the user hasn't
    // enabled it, so it must be hidden.
    return false;
  }

  // None of the filters match, we can display
  VLOG(2) << "None of the filters matched, will display item for publisher "
          << data->publisher_id << ": " << publisher->publisher_name << " ["
          << data->title << "]";
  return true;
}

bool BuildFeed(const std::vector<mojom::FeedItemPtr>& feed_items,
               const std::unordered_set<std::string>& history_hosts,
               Publishers* publishers,
               mojom::Feed* feed,
               const BraveNewsSubscriptions& subscriptions) {
  Channels channels =
      ChannelsController::GetChannelsFromPublishers(*publishers, subscriptions);

  std::list<mojom::ArticlePtr> articles;
  std::list<mojom::PromotedArticlePtr> promoted_articles;
  std::list<mojom::DealPtr> deals;
  std::hash<std::string> hasher;
  base::flat_set<GURL> seen_articles;

  for (auto& item : feed_items) {
    if (!ShouldDisplayFeedItem(item, publishers, channels)) {
      continue;
    }
    auto& metadata = MetadataFromFeedItem(item);
    if (seen_articles.contains(metadata->url)) {
      VLOG(2) << "Skipping " << metadata->url
              << " because we've already seen it.";
      continue;
    }

    seen_articles.insert(metadata->url);
    const auto& publisher = publishers->at(metadata->publisher_id);
    // ShouldDisplayFeedItem should already have returned false
    // if publishers doesn't have this publisher_id.
    DCHECK(publisher);
    // Verify publisher_name field, this is still required for android.
    // TODO(petemill): Have android use publisher_id field and lookup publisher
    // name from its publisher list, so that we can avoid sending this
    // repetitive data over IPC.
    if (metadata->publisher_name.empty()) {
      metadata->publisher_name = publisher->publisher_name;
    }
    // Adjust score to consider profile's browsing history
    if (history_hosts.find(metadata->url.host()) != history_hosts.end()) {
      metadata->score -= 5;
    }

    // Adjust score to consider an explicit follow of the source, vs a
    // channel-based follow
    if (publisher->user_enabled_status ==
        brave_news::mojom::UserEnabled::ENABLED) {
      VLOG(1) << "Found explicit enable, adding score for: "
              << publisher->publisher_name;
      metadata->score -= 10;
    }

    // Get hash at this point since we have a flat list, and our algorithm
    // will only change sorting which can be re-applied on the next
    // feed update.
    feed->hash =
        base::NumberToString(hasher(feed->hash + metadata->url.spec()));
    switch (item->which()) {
      case mojom::FeedItem::Tag::kArticle:
        articles.push_back(std::move(item->get_article()));
        break;
      case mojom::FeedItem::Tag::kDeal:
        deals.push_back(std::move(item->get_deal()));
        break;
      case mojom::FeedItem::Tag::kPromotedArticle:
        promoted_articles.push_back(std::move(item->get_promoted_article()));
        break;
    }
  }
  VLOG(1) << "Got articles # " << articles.size();
  VLOG(1) << "Got deals # " << deals.size();
  VLOG(1) << "Got promoted articles # " << promoted_articles.size();
  // Sort by score, ascending
  articles.sort([](mojom::ArticlePtr& a, mojom::ArticlePtr& b) {
    return (a.get()->data->score < b.get()->data->score);
  });
  promoted_articles.sort(
      [](mojom::PromotedArticlePtr& a, mojom::PromotedArticlePtr& b) {
        return (a.get()->data->score < b.get()->data->score);
      });
  deals.sort([](mojom::DealPtr& a, mojom::DealPtr& b) {
    return (a.get()->data->score < b.get()->data->score);
  });
  // Get unique categories present with article counts
  std::map<std::string, std::int32_t> category_counts;
  for (auto const& article : articles) {
    auto category = article->data->category_name;
    if (!category.empty() && category != "Top News") {
      auto existing_count = category_counts[category];
      category_counts[category] = existing_count + 1;
    }
  }
  // Ordered by # of occurrences
  std::vector<std::string> category_names_by_priority;
  for (auto kv : category_counts) {
    // Top News is always first category
    // TODO(petemill): handle translated version in non-english feeds
    if (kv.first != "Top News") {
      category_names_by_priority.emplace_back(kv.first);
    }
  }
  std::sort(category_names_by_priority.begin(),
            category_names_by_priority.end(),
            [category_counts](std::string& a, std::string& b) {
              return (category_counts.at(a) < category_counts.at(b));
            });
  // Top News is always first category
  category_names_by_priority.insert(category_names_by_priority.begin(),
                                    "Top News");
  VLOG(1) << "Got categories # " << category_names_by_priority.size();
  // Get unique deals categories present
  std::map<std::string, std::int32_t> deal_category_counts;
  for (auto const& deal : deals) {
    auto category = deal->offers_category;
    if (!category.empty()) {
      auto existing_count = category_counts[category];
      category_counts[category] = existing_count + 1;
    }
  }
  // Ordered by # of occurrences
  std::vector<std::string> deal_category_names_by_priority;
  for (auto kv : deal_category_counts) {
    deal_category_names_by_priority.emplace_back(kv.first);
  }
  std::sort(deal_category_names_by_priority.begin(),
            deal_category_names_by_priority.end(),
            [deal_category_counts](std::string& a, std::string& b) {
              return (deal_category_counts.at(a) < deal_category_counts.at(b));
            });
  VLOG(1) << "Got deal categories # " << deal_category_names_by_priority.size();

  // Get first headline
  std::list<mojom::ArticlePtr>::iterator featured_article_it;
  for (featured_article_it = articles.begin();
       featured_article_it != articles.end(); featured_article_it++) {
    // Get the highest score "news" article
    if (featured_article_it->get()->data->category_name == "Top News") {
      VLOG(1) << "Featured item was set to a \"Top News\" article";
      break;
    }
  }
  if (featured_article_it == articles.end()) {
    // If there was no matching "news" article,
    // get the highest score article.
    featured_article_it = articles.begin();
    VLOG(1) << "Featured item was set to the highest ranked article";
  }
  // When we have no articles, do not set a featured item
  if (featured_article_it != articles.end()) {
    auto item = *std::make_move_iterator(featured_article_it);
    auto article = mojom::FeedItem::NewArticle(std::move(item));
    feed->featured_item = std::move(article);
    articles.erase(featured_article_it);
  } else {
    VLOG(1) << "No featured item was set as there are no articles";
  }

  // Generate as many pages of content as possible
  // Make the pages
  int cur_page = 0;
  const int max_pages = 4000;
  auto category_it = category_names_by_priority.begin();
  auto deal_category_it = deal_category_names_by_priority.begin();
  while (cur_page++ < max_pages) {
    if (articles.size() == 0) {
      // No more pages of content
      break;
    }
    std::string deal_category_name =
        (deal_category_it != deal_category_names_by_priority.end())
            ? *deal_category_it
            : "";
    std::string article_category_name =
        (category_it != category_names_by_priority.end()) ? *category_it : "";
    auto feed_page = mojom::FeedPage::New();
    for (auto card_type : g_page_content_order) {
      auto feed_page_item = mojom::FeedPageItem::New();
      feed_page_item->card_type = card_type;
      BuildFeedPageItem(&articles, &promoted_articles, &deals,
                        deal_category_name, article_category_name, false,
                        &feed_page_item);
      feed_page->items.push_back(std::move(feed_page_item));
    }
    for (auto card_type : g_random_content_order) {
      auto feed_page_item = mojom::FeedPageItem::New();
      feed_page_item->card_type = card_type;
      BuildFeedPageItem(&articles, &promoted_articles, &deals,
                        deal_category_name, article_category_name, true,
                        &feed_page_item);
      feed_page->items.push_back(std::move(feed_page_item));
    }
    feed->pages.push_back(std::move(feed_page));
    if (!deal_category_name.empty()) {
      deal_category_it++;
    }
    if (!article_category_name.empty()) {
      category_it++;
    }
  }
  VLOG(1) << "Made pages # " << feed->pages.size();
  return true;
}

}  // namespace brave_news
