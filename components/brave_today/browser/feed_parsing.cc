// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/feed_parsing.h"

#include <codecvt>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/time_format.h"
#include "url/gurl.h"

namespace brave_news {

namespace {

bool ParseFeedItem(const base::Value& feed_item_raw,
                   mojom::FeedItemPtr* feed_item) {
  auto url_raw = *feed_item_raw.FindStringKey("url");
  if (url_raw.empty()) {
    VLOG(1) << "Found feed item with missing url. Title: "
            << *feed_item_raw.FindStringKey("title");
    return false;
  }
  auto image_url_raw = *feed_item_raw.FindStringKey("padded_img");
  // Filter out non-image articles
  if (image_url_raw.empty()) {
    VLOG(2) << "Found feed item with missing image. Url: " << url_raw;
    return false;
  }
  auto publisher_id = *feed_item_raw.FindStringKey("publisher_id");
  if (publisher_id.empty()) {
    VLOG(1) << "Found article with missing publisher_id. Url: " << url_raw;
    return false;
  }
  // Parse metadata which all content types have
  auto metadata = mojom::FeedItemMetadata::New();
  metadata->category_name = *feed_item_raw.FindStringKey("category");
  metadata->title = *feed_item_raw.FindStringKey("title");
  // Title is mandatory
  if (metadata->title.empty()) {
    VLOG(2) << "Item was missing a title: " << url_raw;
    return false;
  }
  metadata->description = *feed_item_raw.FindStringKey("description");
  metadata->publisher_id = publisher_id;
  metadata->publisher_name = *feed_item_raw.FindStringKey("publisher_name");
  auto image_url = mojom::Image::NewPaddedImageUrl(GURL(image_url_raw));
  metadata->image = std::move(image_url);
  auto url = GURL(url_raw);
  if (url.is_empty() || !url.has_host()) {
    VLOG(1) << "Could not parse item url: " << url_raw;
    return false;
  }
  metadata->url = std::move(url);
  // Further weight according to history
  auto score = feed_item_raw.FindDoubleKey("score");
  if (!score.has_value()) {
    VLOG(1) << "Item was missing score: " << url_raw;
  }
  metadata->score = score.value_or(20.0);
  // Extract time
  const char* publish_time_raw =
      (*feed_item_raw.FindStringKey("publish_time")).c_str();
  if (!base::Time::FromUTCString(publish_time_raw, &metadata->publish_time)) {
    VLOG(1) << "bad time string for feed item: " << publish_time_raw;
  } else {
    // Successful, get language-specific relative time
    base::TimeDelta relative_time_delta =
        base::Time::Now() - metadata->publish_time;
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
    metadata->relative_time_description =
        converter.to_bytes(ui::TimeFormat::Simple(
            ui::TimeFormat::Format::FORMAT_ELAPSED,
            ui::TimeFormat::Length::LENGTH_LONG, relative_time_delta));
  }
  // Detect type
  auto content_type = *feed_item_raw.FindStringKey("content_type");
  if (content_type == "brave_partner") {
    auto item = mojom::PromotedArticle::New();
    item->creative_instance_id =
        *feed_item_raw.FindStringKey("creative_instance_id");
    if (item->creative_instance_id.empty()) {
      VLOG(1) << "Promoted Item has empty creative_instance_id: " << url_raw;
      return false;
    }
    item->data = std::move(metadata);
    feed_item->get()->set_promoted_article(std::move(item));
  } else if (content_type == "product") {
    auto item = mojom::Deal::New();
    item->offers_category = *feed_item_raw.FindStringKey("offers_category");
    item->data = std::move(metadata);
    feed_item->get()->set_deal(std::move(item));
  } else if (content_type == "article") {
    auto item = mojom::Article::New();
    item->data = std::move(metadata);
    feed_item->get()->set_article(std::move(item));
  } else {
    // Do not error if unknown content_type is discovered, it could
    // be a future use.
    VLOG(3) << "Unknown content type of: " << content_type;
    return false;
  }
  return true;
}

}  // namespace

bool ParseFeedItems(const std::string& json,
                    std::vector<mojom::FeedItemPtr>* feed_items) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  if (!records_v->is_list()) {
    return false;
  }
  for (const base::Value& feed_item_raw : records_v->GetListDeprecated()) {
    auto item = mojom::FeedItem::New();
    std::string item_hash;
    if (ParseFeedItem(feed_item_raw, &item)) {
      feed_items->push_back(std::move(item));
    }
  }
  return true;
}

}  // namespace brave_news
