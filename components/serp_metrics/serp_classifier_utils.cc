/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_classifier_utils.h"

#include <string>

#include "base/containers/fixed_flat_set.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace serp_metrics {

namespace {

constexpr auto kAllowedSearchEngines = base::MakeFixedFlatSet<SearchEngineType>(
    base::sorted_unique,
    {SEARCH_ENGINE_BING, SEARCH_ENGINE_GOOGLE, SEARCH_ENGINE_YAHOO,
     SEARCH_ENGINE_DUCKDUCKGO, SEARCH_ENGINE_QWANT, SEARCH_ENGINE_ECOSIA,
     SEARCH_ENGINE_BRAVE, SEARCH_ENGINE_STARTPAGE});

// Google URL query parameter names that select a search vertical.
// Known `tbm` verticals: isch (images), nws (news), vid (video), shop
// (shopping), bks (books).
// Known `udm` verticals: 2 (images), 7 (video), 14 (shopping), 18
// (discussions).
constexpr std::string_view kTbmParam = "tbm";
constexpr std::string_view kUdmParam = "udm";

// `udm` values that represent a regular web search rather than a vertical.
// 0 is the implicit default; 28 is the explicit Web tab.
constexpr std::string_view kUdmAllResults = "0";
constexpr std::string_view kUdmWebTab = "28";

}  // namespace

bool IsAllowedSearchEngine(SearchEngineType type) {
  return kAllowedSearchEngines.contains(type);
}

bool IsGoogleWebSearch(const GURL& url) {
  std::string value;
  if (net::GetValueForKeyInQuery(url, kTbmParam, &value)) {
    // Any `tbm` value routes to a vertical search (images, news, video, etc.).
    return false;
  }
  if (net::GetValueForKeyInQuery(url, kUdmParam, &value) &&
      value != kUdmAllResults && value != kUdmWebTab) {
    // A non-zero `udm` routes to a vertical search (shopping, etc.), except
    // `udm=28` which is the explicit Web tab and still represents a web search.
    return false;
  }
  return true;
}

}  // namespace serp_metrics
