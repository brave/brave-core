/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SEARCH_ENGINE_UTIL_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SEARCH_ENGINE_UTIL_H_

#include <optional>

class GURL;

namespace metrics {

struct SearchEngineInfo;

// Returns `true` if `url` represents a search engine results page for a known
// search engine.
bool IsSearchEngineResultsPage(const GURL& url);

// Returns metadata describing the search engine associated with `url`, if `url`
// belongs to a known search engine. If the URL does not match any supported
// search engine, `std::nullopt` is returned.
std::optional<SearchEngineInfo> MaybeGetSearchEngine(const GURL& url);

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SEARCH_ENGINE_UTIL_H_
