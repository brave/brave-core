// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/combined_feed_parsing.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/brave_news/api/combined_feed.h"
#include "brave/components/brave_news/browser/channel_migrator.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/features.h"
#include "ui/base/l10n/time_format.h"
#include "url/gurl.h"

namespace brave_news {

namespace {

base::expected<mojom::FeedItemPtr, std::string> ParseFeedItem(
    const base::Value& value) {
  auto parsed_feed_item = api::combined_feed::Item::FromValue(value);
  if (!parsed_feed_item.has_value()) {
    return base::unexpected(
        base::StrCat({"Failed to parse feed item. ",
                      base::UTF16ToASCII(parsed_feed_item.error())}));
  }
  api::combined_feed::Item& feed_item = *parsed_feed_item;

  auto url = GURL(feed_item.url);
  if (url.is_empty() || !url.has_host()) {
    return base::unexpected(base::StrCat(
        {"Found feed item with an invalid url value. title=", feed_item.title,
         ", url=", feed_item.url}));
  }

  if (!url.SchemeIsHTTPOrHTTPS()) {
    return base::unexpected(
        base::StrCat({"Item url was not HTTP or HTTPS: url=", url.spec()}));
  }

  // FeedV2 supports articles with no images, such as the ones from Brave Blog.
  if (!base::FeatureList::IsEnabled(
          brave_news::features::kBraveNewsFeedUpdate)) {
    if (feed_item.padded_img.empty()) {
      return base::unexpected(base::StrCat(
          {"Found feed item with missing image. url=", feed_item.url}));
    }
  }

  if (feed_item.publisher_id.empty()) {
    return base::unexpected(base::StrCat(
        {"Found feed item with missing publisher id. url=", feed_item.url}));
  }

  if (feed_item.title.empty()) {
    return base::unexpected(base::StrCat(
        {"Found feed item with missing title. url=", feed_item.url}));
  }

  if (!feed_item.score.has_value()) {
    // We just warn, as this is an optional field.
    VLOG(1) << "Item was missing score: " << feed_item.url;
  }

  auto metadata = mojom::FeedItemMetadata::New();
  metadata->category_name = GetMigratedChannel(feed_item.category);
  if (feed_item.channels) {
    std::ranges::transform(*feed_item.channels,
                           std::back_inserter(metadata->channels),
                           &GetMigratedChannel);
  }

  metadata->title = feed_item.title;
  metadata->description = feed_item.description;
  metadata->publisher_id = feed_item.publisher_id;
  metadata->publisher_name = feed_item.publisher_name;
  metadata->image = mojom::Image::NewPaddedImageUrl(GURL(feed_item.padded_img));
  metadata->url = std::move(url);

  // Further weight according to history
  metadata->score = feed_item.score.value_or(20.0);
  metadata->pop_score = feed_item.pop_score.value_or(0.);

  // Extract time
  if (!base::Time::FromUTCString(feed_item.publish_time.c_str(),
                                 &metadata->publish_time)) {
    VLOG(1) << "Bad time string for feed item: " << feed_item.publish_time;
  } else {
    // Successful, get language-specific relative time
    base::TimeDelta relative_time_delta =
        base::Time::Now() - metadata->publish_time;
    metadata->relative_time_description =
        base::UTF16ToUTF8(ui::TimeFormat::Simple(
            ui::TimeFormat::Format::FORMAT_ELAPSED,
            ui::TimeFormat::Length::LENGTH_LONG, relative_time_delta));
  }
  // Detect type
  if (feed_item.content_type == "brave_partner") {
    if (!feed_item.creative_instance_id ||
        feed_item.creative_instance_id->empty()) {
      return base::unexpected(
          base::StrCat({"Promoted item has empty creative_instance_id. url=",
                        feed_item.url}));
    }

    auto item = mojom::PromotedArticle::New();
    item->creative_instance_id = *feed_item.creative_instance_id;
    item->data = std::move(metadata);
    return mojom::FeedItem::NewPromotedArticle(std::move(item));
  } else if (feed_item.content_type == "product") {
    auto item = mojom::Deal::New();
    if (feed_item.offers_category) {
      item->offers_category = *feed_item.offers_category;
    }
    item->data = std::move(metadata);
    return mojom::FeedItem::NewDeal(std::move(item));
  } else if (feed_item.content_type == "article") {
    auto item = mojom::Article::New();
    item->data = std::move(metadata);
    return mojom::FeedItem::NewArticle(std::move(item));
  }

  // An unknown content_type could be something introduced for future use.
  return base::unexpected(
      base::StrCat({"Feed item of unknown content type. content_type=",
                    feed_item.content_type}));
}

}  // namespace

std::vector<mojom::FeedItemPtr> ParseFeedItems(const base::Value& value) {
  std::vector<mojom::FeedItemPtr> items;
  if (!value.is_list()) {
    NOTREACHED_IN_MIGRATION();
    return items;
  }
  for (const base::Value& feed_item : value.GetList()) {
    auto item = ParseFeedItem(feed_item);
    if (item.has_value()) {
      items.push_back(std::move(*item));
    } else {
      VLOG(1) << item.error();
    }
  }
  return items;
}

}  // namespace brave_news
