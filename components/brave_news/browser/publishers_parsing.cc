// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/publishers_parsing.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_news/api/publisher.h"
#include "brave/components/brave_news/browser/channel_migrator.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "url/gurl.h"

namespace brave_news {

std::optional<Publishers> ParseCombinedPublisherList(const base::Value& value) {
  if (!value.is_list()) {
    LOG(ERROR) << "Publisher data expected to be a list";
    return std::nullopt;
  }

  Publishers result;

  for (const base::Value& publisher_value : value.GetList()) {
    auto parsed_publisher = api::feed::Publisher::FromValue(publisher_value);
    if (!parsed_publisher.has_value()) {
      LOG(ERROR) << "Invalid Brave Publisher data. error="
                 << parsed_publisher.error();
      return std::nullopt;
    }
    auto& entry = *parsed_publisher;

    GURL site_url = [&entry]() {
      if (base::StartsWith(entry.site_url, "https://")) {
        return GURL(entry.site_url);
      } else {
        return GURL("https://" + entry.site_url);
      }
    }();

    if (!site_url.is_valid()) {
      LOG(ERROR) << "Found invalid site url for Brave News publisher "
                 << entry.publisher_name << "(was " << entry.site_url << ")";
      continue;
    }

    auto publisher = brave_news::mojom::Publisher::New();

    publisher->site_url = site_url;
    publisher->publisher_id = entry.publisher_id;
    publisher->type = mojom::PublisherType::COMBINED_SOURCE;
    publisher->publisher_name = entry.publisher_name;
    publisher->category_name = entry.category;
    publisher->is_enabled = entry.enabled.value_or(true);
    GURL feed_source(entry.feed_url);
    if (feed_source.is_valid()) {
      publisher->feed_source = feed_source;
    }

    if (entry.locales) {
      for (auto& locale : *entry.locales) {
        auto locale_info = mojom::LocaleInfo::New();
        locale_info->locale = locale.locale;
        locale_info->rank = locale.rank.value_or(0);

        // With migrations, it's possible we'll end up with duplicate channels,
        // so filter them out with a set.
        base::flat_set<std::string> seen;
        for (const auto& channel : locale.channels) {
          auto transformed = brave_news::GetMigratedChannel(channel);
          if (seen.contains(transformed)) {
            continue;
          }
          seen.insert(transformed);
          locale_info->channels.push_back(std::move(transformed));
        }

        publisher->locales.push_back(std::move(locale_info));
      }
    }

    if (entry.favicon_url) {
      if (GURL favicon_url(*entry.favicon_url); favicon_url.is_valid()) {
        publisher->favicon_url = favicon_url;
      }
    }

    if (entry.cover_url) {
      if (GURL cover_url(*entry.cover_url); cover_url.is_valid()) {
        publisher->cover_url = cover_url;
      }
    }

    if (entry.background_color) {
      publisher->background_color = *entry.background_color;
    }

    // TODO(petemill): Validate
    result.insert_or_assign(entry.publisher_id, std::move(publisher));
  }
  return result;
}

void ParseDirectPublisherList(const base::Value::Dict& direct_feeds_pref_dict,
                              std::vector<mojom::PublisherPtr>* publishers) {
  for (const auto&& [key, value] : direct_feeds_pref_dict) {
    const base::Value::Dict* root = value.GetIfDict();
    if (!root) {
      // Handle unknown value type
      LOG(ERROR) << "Found unknown dictionary pref value for"
                    "Brave News direct feeds at the pref path: "
                 << key;
      // TODO(petemill): delete item from pref dict?
      continue;
    }
    VLOG(1) << "Found direct feed in prefs: " << key;

    GURL feed_source(*root->FindString(prefs::kBraveNewsDirectFeedsKeySource));
    if (!feed_source.is_valid()) {
      // This is worth error logging because we shouldn't
      // get in to this state due to validation at the
      // point of adding the item to prefs.
      LOG(ERROR) << "Found invalid feed url for Brave News "
                    "direct feeds pref at the path "
                 << prefs::kBraveNewsDirectFeeds << " > "
                 << prefs::kBraveNewsDirectFeedsKeySource;
      // TODO(petemill): delete item from pref dict?
      continue;
    }
    auto publisher = mojom::Publisher::New();
    publisher->feed_source = feed_source;
    publisher->publisher_id = key;
    publisher->publisher_name =
        *root->FindString(prefs::kBraveNewsDirectFeedsKeyTitle);
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
