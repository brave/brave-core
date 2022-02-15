// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/publishers_parsing.h"

#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "brave/components/brave_today/common/pref_names.h"

namespace brave_news {

bool ParseCombinedPublisherList(const std::string& json,
                                Publishers* publishers) {
  DCHECK(publishers);
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  if (!records_v->is_list()) {
    return false;
  }
  for (const base::Value& publisher_raw : records_v->GetListDeprecated()) {
    auto publisher = brave_news::mojom::Publisher::New();
    publisher->publisher_id = *publisher_raw.FindStringKey("publisher_id");
    publisher->type = mojom::PublisherType::COMBINED_SOURCE;
    publisher->publisher_name = *publisher_raw.FindStringKey("publisher_name");
    publisher->category_name = *publisher_raw.FindStringKey("category");
    publisher->is_enabled = publisher_raw.FindBoolKey("enabled").value_or(true);
    // TODO(petemill): Validate
    publishers->insert_or_assign(publisher->publisher_id, std::move(publisher));
  }
  return true;
}

void ParseDirectPublisherList(const base::Value* direct_feeds_pref_value,
                              std::vector<mojom::PublisherPtr>* publishers) {
  for (auto const kv : direct_feeds_pref_value->DictItems()) {
    if (!kv.second.is_dict()) {
      // Handle unknown value type
      LOG(ERROR) << "Found unknown dictionary pref value for"
                    "Brave News direct feeds at the pref path: "
                 << kv.first;
      // TODO(petemill): delete item from pref dict?
      continue;
    }
    VLOG(1) << "Found direct feed in prefs: " << kv.first;
    const auto& value = kv.second;
    GURL feed_source(
        *value.FindStringKey(prefs::kBraveTodayDirectFeedsKeySource));
    if (!feed_source.is_valid()) {
      // This is worth error logging because we shouldn't
      // get in to this state due to validation at the
      // point of adding the item to prefs.
      LOG(ERROR) << "Found invalid feed url for Brave News "
                    "direct feeds pref at the path "
                 << prefs::kBraveTodayDirectFeeds << " > "
                 << prefs::kBraveTodayDirectFeedsKeySource;
      // TODO(petemill): delete item from pref dict?
      continue;
    }
    auto publisher = mojom::Publisher::New();
    publisher->feed_source = feed_source;
    publisher->publisher_id = kv.first;
    publisher->publisher_name =
        *value.FindStringKey(prefs::kBraveTodayDirectFeedsKeyTitle);
    publisher->type = mojom::PublisherType::DIRECT_SOURCE;
    // This is always true for direct feeds, reserved property for
    // "combined source" feeds, and perhaps marking a direct feed
    // as "bad".
    publisher->is_enabled = true;
    // TODO(petemill): Allow the user to disable but not delete a feed
    publisher->user_enabled_status = mojom::UserEnabled::NOT_MODIFIED;
    publishers->emplace_back(std::move(publisher));
  }
}

}  // namespace brave_news
