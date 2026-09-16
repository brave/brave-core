/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_RELEVANT_SITE_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_RELEVANT_SITE_H_

#include <optional>
#include <string_view>

namespace web_discovery {

enum class RelevantSite {
  kGoogleImages,   // search-goi
  kGoogleVideos,   // search-gov
  kGoogle,         // search-go
  kYahoo,          // search-ya
  kBingImages,     // search-bii
  kBing,           // search-bi
  kAmazonSearch,   // search-am
  kAmazonProduct,  // amp
  kDuckDuckGo,     // search-dd
  kLinkedIn,       // li
};

// Converts an ID to the corresponding RelevantSite enum value
std::optional<RelevantSite> RelevantSiteFromID(std::string_view id);

// Converts a RelevantSite enum value to its corresponding ID
std::optional<std::string_view> RelevantSiteToID(RelevantSite site);

// Determines if a RelevantSite is a search engine
bool IsRelevantSiteSearchEngine(RelevantSite site);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_RELEVANT_SITE_H_
