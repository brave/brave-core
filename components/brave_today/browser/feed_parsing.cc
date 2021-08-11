// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/feed_parsing.h"

#include <algorithm>
#include <codecvt>
#include <cstdint>
#include <iterator>
#include <list>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "components/history/core/browser/history_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/time_format.h"

namespace brave_news {

bool ParsePublisherList(const std::string& json,
                        Publishers* publishers) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  const base::ListValue* response_list;
  if (!records_v->GetAsList(&response_list)) {
    return false;
  }
  for (const base::Value& publisher_raw : response_list->GetList()) {
    auto publisher = brave_news::mojom::Publisher::New();
    publisher->publisher_id = *publisher_raw.FindStringKey("publisher_id");
    publisher->publisher_name = *publisher_raw.FindStringKey("publisher_name");
    publisher->category_name = *publisher_raw.FindStringKey("category");
    publisher->is_enabled = publisher_raw.FindBoolKey("enabled").value_or(true);
    // TODO(petemill): Validate
    publishers->insert_or_assign(publisher->publisher_id, std::move(publisher));
  }
  return true;
}

bool ParseFeed(const std::string& json,
                        Publishers* publishers,
                        std::unordered_set<std::string>& history_hosts,
                        mojom::Feed* feed) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  const base::ListValue* response_list;
  if (!records_v->GetAsList(&response_list)) {
    return false;
  }
  std::list<mojom::ArticlePtr> articles;
  std::list<mojom::PromotedArticlePtr> promoted_articles;
  std::list<mojom::DealPtr> deals;
  std::hash<std::string> hasher;
  for (const base::Value& feed_item_raw : response_list->GetList()) {
    auto url_raw = *feed_item_raw.FindStringKey("url");
    if (url_raw.empty()) {
      VLOG(1) << "Found feed item with missing url. Title: "
          << *feed_item_raw.FindStringKey("title");
      continue;
    }
    auto image_url_raw = *feed_item_raw.FindStringKey("padded_img");
    // Filter out non-image articles
    if (image_url_raw.empty()) {
      VLOG(2) << "Found feed item with missing image. Url: " << url_raw;
      continue;
    }
    // Filter out articles from publishers we're ignoring
    auto publisher_id = *feed_item_raw.FindStringKey("publisher_id");
    if (publisher_id.empty()) {
      VLOG(1) << "found article with missing publisher_id. Url: " << url_raw;
      continue;
    }
    if (!publishers->contains(publisher_id)) {
      VLOG(1) << "found article with unknown publisher_id. PublisherId: "
          << publisher_id << " Url: " << url_raw;
      continue;
    }
    if (publishers->at(publisher_id)->user_enabled_status ==
            brave_news::mojom::UserEnabled::DISABLED) {
      VLOG(1) << "Hiding article for publisher " << publisher_id << ": " << publishers->at(publisher_id)->publisher_name;
      continue;
    }
    // Parse metadata which all content types have
    auto metadata = mojom::FeedItemMetadata::New();
    metadata->category_name = *feed_item_raw.FindStringKey("category");
    metadata->title = *feed_item_raw.FindStringKey("title");
    metadata->description = *feed_item_raw.FindStringKey("description");
    metadata->publisher_id = publisher_id;
    metadata->publisher_name = *feed_item_raw.FindStringKey("publisher_name");
    auto image_url = mojom::Image::NewPaddedImageUrl(
      GURL(image_url_raw));
    metadata->image = std::move(image_url);
    auto url = GURL(url_raw);
    if (url.is_empty() || !url.has_host()) {
      VLOG(1) << "Could not parse item url: " << url_raw;
      continue;
    }
    metadata->url = std::move(url);
    // Further weight according to history
    metadata->score = feed_item_raw.FindDoubleKey("score").value_or(0.0);
    if (history_hosts.find(metadata->url.host()) != history_hosts.end()) {
      metadata->score-=5;
    }
    // Extract time
    const char* publish_time_raw = (*feed_item_raw.FindStringKey("publish_time")).c_str();
    if (!base::Time::FromUTCString(publish_time_raw, &metadata->publish_time)) {
      VLOG(1) << "bad time string for feed item: " << publish_time_raw;
    } else {
      // Successful, get language-specific relative time
      base::TimeDelta relative_time_delta = base::Time::Now() - metadata->publish_time;
      std::wstring_convert<
          std::codecvt_utf8_utf16<char16_t>,char16_t> converter;
      metadata->relative_time_description = converter.to_bytes(ui::TimeFormat::Simple(
          ui::TimeFormat::Format::FORMAT_ELAPSED,
          ui::TimeFormat::Length::LENGTH_LONG, relative_time_delta));
    }
    // Get hash at this point since we have a flat list, and our algorithm
    // will only change sorting which can be re-applied on the next
    // feed update.
    feed->hash = std::to_string(hasher(feed->hash + url_raw));
    // TODO(petemill): validate
    // Detect type
    auto content_type = *feed_item_raw.FindStringKey("content_type");
    if (content_type == "brave_partner") {
        auto item = mojom::PromotedArticle::New();
        item->creative_instance_id = *feed_item_raw.FindStringKey("creative_instance_id");
        item->data = std::move(metadata);
        promoted_articles.push_back(std::move(item));
    } else if (content_type == "product") {
        auto item = mojom::Deal::New();
        item->offers_category = *feed_item_raw.FindStringKey("offers_category");
        item->data = std::move(metadata);
        deals.push_back(std::move(item));
    } else if (content_type == "article") {
        auto item = mojom::Article::New();
        item->data = std::move(metadata);
        articles.push_back(std::move(item));
    }
    // Do not error if unknown content_type is discovered, it could
    // be a future use.
  }
  VLOG(1) << "Got articles # " << articles.size();
  VLOG(1) << "Got deals # " << deals.size();
  VLOG(1) << "Got promoted articles # " << promoted_articles.size();
  // Sort by score
  articles.sort([](mojom::ArticlePtr &a, mojom::ArticlePtr &b) {
    return (a.get()->data->score < b.get()->data->score);
  });
  promoted_articles.sort([](mojom::PromotedArticlePtr &a, mojom::PromotedArticlePtr &b) {
    return (a.get()->data->score < b.get()->data->score);
  });
  deals.sort([](mojom::DealPtr &a, mojom::DealPtr &b) {
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
  // Ordered by # of occurances
  std::vector<std::string> category_names_by_priority;
  for (auto kv : category_counts) {
    // Top News is always first category
    if (kv.first != "Top News")
      category_names_by_priority.emplace_back(kv.first);
  }
  std::sort(category_names_by_priority.begin(),
    category_names_by_priority.end(),
    [category_counts](std::string &a, std::string &b) {
      return (category_counts.at(a) < category_counts.at(b));
    });
  // Top News is always first category
  category_names_by_priority.insert(
      category_names_by_priority.begin(), "Top News");
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
  // Ordered by # of occurances
  std::vector<std::string> deal_category_names_by_priority;
  for (auto kv : deal_category_counts) {
    deal_category_names_by_priority.emplace_back(kv.first);
  }
  std::sort(deal_category_names_by_priority.begin(),
      deal_category_names_by_priority.end(),
      [deal_category_counts](std::string &a, std::string &b) {
        return (deal_category_counts.at(a) < deal_category_counts.at(b));
      });
  VLOG(1) << "Got deal categories # " << deal_category_names_by_priority.size();
  // Get first headline
  std::list<mojom::ArticlePtr>::iterator featured_article_it;
  for (featured_article_it = articles.begin();
          featured_article_it != articles.end(); featured_article_it++) {
    if (featured_article_it->get()->data->category_name == "Top News") {
      break;
    }
  }
  if (featured_article_it != articles.end()) {
    auto item = *std::make_move_iterator(featured_article_it);
    feed->featured_article = std::move(item);
    articles.erase(featured_article_it);
  }
  // Generate as many pages of content as possible
  // Make the pages
  int cur_page = 0;
  const int max_pages = 4000;
  auto category_it = category_names_by_priority.begin();
  auto deal_category_it = deal_category_names_by_priority.begin();
  auto promoted_it = promoted_articles.begin();
  while (cur_page++ < max_pages) {
    auto page = mojom::Page::New();
    // Collect headlines
    // TODO(petemill): Use the CardType type and PageContentOrder array
    // from cardsGroup.tsx here instead of having to synchronise the amount
    // of articles we take per page with how many get rendered.
    // Or generate the pages on the frontend and just provide the data in 1 array
    // here.
    size_t n_articles = 15;
    auto end = std::next(articles.begin(), std::min(n_articles, articles.size()));
    std::list<mojom::ArticlePtr> headlines;
    headlines.splice(headlines.begin(), articles, articles.begin(), end);
    if (headlines.size() == 0) {
      // No more pages of content
      break;
    }
    page->articles.insert(page->articles.end(),
        std::make_move_iterator(headlines.begin()),
                  std::make_move_iterator(headlines.end()));
    // Collect category
    if (category_it != category_names_by_priority.end()) {
      std::string category_name = *category_it;
      page->items_by_category = mojom::CategoryArticles::New();
      page->items_by_category->category_name = category_name;
      std::list<mojom::ArticlePtr> category_articles;
      std::list<mojom::ArticlePtr>::iterator it;
      for (it = articles.begin();
             it != articles.end() && category_articles.size() < 3;
             it++) {
        if (it->get()->data->category_name == page->items_by_category->category_name) {
          // Pass a copy of the iterator (via `it--`) so that we move
          // forward with one that hasn't been removed.
          category_articles.splice(category_articles.end(), articles, it--);
        }
      }
      page->items_by_category->articles.insert(page->items_by_category->articles.end(),
          std::make_move_iterator(category_articles.begin()),
          std::make_move_iterator(category_articles.end()));
      category_it++;
    }
    // Collect deals
    std::list<mojom::DealPtr> page_deals;
    const auto desired_deal_count = 3u;
    if (deal_category_it != deal_category_names_by_priority.end()) {
      std::string deal_category_name = *deal_category_it;
      // Increment for next page
      deal_category_it++;
      std::list<mojom::DealPtr>::iterator it;
      for (it = deals.begin();
              it != deals.end() && page_deals.size() < desired_deal_count;
              it++) {
        if (it->get()->offers_category == deal_category_name) {
          page_deals.splice(page_deals.end(), deals, it--);
        }
      }
    }
    // Supplement with deals from other categories if we end up with fewer
    // than we want
    if (page_deals.size() < desired_deal_count) {
      auto supplemental_deal_count = std::min<uint>(
          desired_deal_count, deals.size());
      if (supplemental_deal_count > 0) {
        auto begin = deals.begin();
        auto end = std::next(deals.begin(), supplemental_deal_count);
        page_deals.splice(page_deals.end(), deals, begin, end);
      }
    }
    if (page_deals.size()) {
      std::vector<mojom::DealPtr> page_deals_v;
      page_deals_v.insert(page_deals_v.end(),
          std::make_move_iterator(page_deals.begin()),
          std::make_move_iterator(page_deals.end()));
      page->deals = std::move(page_deals_v);
    }
    // Items by publisher
    std::string page_publisher_id;
    std::list<mojom::ArticlePtr> page_publisher_articles;
    std::list<mojom::ArticlePtr>::iterator it;
    const auto desired_publisher_group_count = 3u;
    for (it = articles.begin();
            it != articles.end() &&
                page_publisher_articles.size() < desired_publisher_group_count;
            it++) {
      if (it->get()->data->publisher_id.empty()) {
        continue;
      }
      // Choose the first publisher available
      if (page_publisher_id.empty()) {
        page_publisher_id = it->get()->data->publisher_id;
      }
      // take this article and a few more
      if (it->get()->data->publisher_id == page_publisher_id) {
        page_publisher_articles.splice(
            page_publisher_articles.end(), articles, it--);
      }
    }
    if (page_publisher_articles.size()) {
      page->items_by_publisher = mojom::PublisherArticles::New();
      page->items_by_publisher->publisher_id = page_publisher_id;
      page->items_by_publisher->articles.insert(
          page->items_by_publisher->articles.end(),
          std::make_move_iterator(page_publisher_articles.begin()),
          std::make_move_iterator(page_publisher_articles.end()));
    }
    // Promoted articles
    if (promoted_it != promoted_articles.end()) {
      // Remove 1 item at the beginning
      auto i = std::make_move_iterator(promoted_it);
      auto item = *i;
      page->promoted_article = std::move(item);
      promoted_articles.erase(promoted_it);
      promoted_it = promoted_articles.begin();
    }
    // Random articles
    const auto desired_random_articles_count = 3u;
    std::list<mojom::ArticlePtr>::iterator random_it;
    std::vector<std::list<mojom::ArticlePtr>::iterator> matching_iterators;
    base::Time time_limit = base::Time::Now() - base::TimeDelta::FromDays(2);
    for (random_it = articles.begin();
            random_it != articles.end();
            random_it++) {
      // Only if article within last 48hrs
      if (random_it->get()->data->publish_time >= time_limit) {
        matching_iterators.push_back(random_it);
      }
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(matching_iterators.begin(), matching_iterators.end(), g);
    if (matching_iterators.size() > desired_random_articles_count) {
      matching_iterators.resize(desired_random_articles_count);
    }
    for (auto & matching_iterator : matching_iterators) {
      auto a = std::make_move_iterator(matching_iterator);
      auto item = *a;
      page->random_articles.push_back(std::move(item));
      articles.erase(matching_iterator);
    }
    feed->pages.push_back(std::move(page));
  }
  VLOG(1) << "Made pages # " << feed->pages.size();
  return true;
}

}  // namespace brave_news