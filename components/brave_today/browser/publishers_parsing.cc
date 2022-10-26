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
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_news {

mojom::LocaleInfoPtr ParseLocaleInfo(const base::Value::Dict& publisher_dict,
                                     const base::Value& locale_entry) {
  auto result = mojom::LocaleInfo::New();

  // TODO(fallaciousreasoining): Remove this branch after sources.global.json
  // has been updated. https://github.com/brave/brave-browser/issues/26307
  if (locale_entry.is_string()) {
    result->locale = locale_entry.GetString();
    result->rank = publisher_dict.FindInt("rank").value_or(0);
    auto* channels_raw = publisher_dict.FindList("channels");
    if (channels_raw) {
      for (const auto& channel : *channels_raw)
        result->channels.push_back(channel.GetString());
    }

    return result;
  }

  const auto& locale_dict = locale_entry.GetDict();
  result->locale = *locale_dict.FindString("locale");
  result->rank = locale_dict.FindInt("rank").value_or(0);

  for (const auto& channel : *locale_dict.FindList("channels"))
    result->channels.push_back(channel.GetString());
  return result;
}

bool ParseCombinedPublisherList(const std::string& json,
                                Publishers* publishers) {
  DCHECK(publishers);
  absl::optional<base::Value> records_v =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                       base::JSONParserOptions::JSON_PARSE_RFC);
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  if (!records_v->is_list()) {
    return false;
  }
  for (const base::Value& publisher_raw : records_v->GetList()) {
    const auto& publisher_dict = publisher_raw.GetDict();

    auto publisher = brave_news::mojom::Publisher::New();
    publisher->publisher_id = *publisher_dict.FindString("publisher_id");
    publisher->type = mojom::PublisherType::COMBINED_SOURCE;
    publisher->publisher_name = *publisher_dict.FindString("publisher_name");

    publisher->category_name = *publisher_dict.FindString("category");
    publisher->is_enabled = publisher_dict.FindBool("enabled").value_or(true);
    GURL feed_source(*publisher_dict.FindString("feed_url"));
    if (feed_source.is_valid()) {
      publisher->feed_source = feed_source;
    }

    auto* locales_raw = publisher_dict.FindList("locales");
    if (locales_raw) {
      for (const auto& locale_raw : *locales_raw) {
        publisher->locales.push_back(
            ParseLocaleInfo(publisher_dict, locale_raw));
      }
    }

    std::string site_url_raw = *publisher_dict.FindString("site_url");
    if (!base::StartsWith(site_url_raw, "https://")) {
      site_url_raw = "https://" + site_url_raw;
    }
    GURL site_url(site_url_raw);
    if (!site_url.is_valid()) {
      LOG(ERROR) << "Found invalid site url for Brave News publisher "
                 << publisher->publisher_name << "(was " << site_url_raw << ")";
      continue;
    }
    publisher->site_url = site_url;

    auto* favicon_url_raw = publisher_dict.FindString("favicon_url");
    if (favicon_url_raw) {
      if (GURL favicon_url(*favicon_url_raw); favicon_url.is_valid()) {
        publisher->favicon_url = favicon_url;
      }
    }

    auto* cover_url_raw = publisher_dict.FindString("cover_url");
    if (cover_url_raw) {
      if (GURL cover_url(*cover_url_raw); cover_url.is_valid()) {
        publisher->cover_url = cover_url;
      }
    }

    auto* background_color = publisher_dict.FindString("background_color");
    if (background_color) {
      publisher->background_color = *background_color;
    }

    // TODO(petemill): Validate
    publishers->insert_or_assign(publisher->publisher_id, std::move(publisher));
  }
  return true;
}

void ParseDirectPublisherList(const base::Value::Dict& direct_feeds_pref_dict,
                              std::vector<mojom::PublisherPtr>* publishers) {
  for (const auto&& [key, value] : direct_feeds_pref_dict) {
    if (!value.is_dict()) {
      // Handle unknown value type
      LOG(ERROR) << "Found unknown dictionary pref value for"
                    "Brave News direct feeds at the pref path: "
                 << key;
      // TODO(petemill): delete item from pref dict?
      continue;
    }
    VLOG(1) << "Found direct feed in prefs: " << key;

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
    publisher->publisher_id = key;
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
