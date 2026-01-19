/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/search_engine_util.h"

#include <string>

#include "brave/components/serp_metrics/search_engine_constants.h"
#include "brave/components/serp_metrics/search_engine_info.h"
#include "net/base/url_util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace metrics {

bool IsSearchEngineResultsPage(const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }

  std::optional<SearchEngineInfo> search_engine = MaybeGetSearchEngine(url);
  if (!search_engine) {
    return false;
  }

  GURL::Replacements replacements;
  replacements.ClearQuery();
  const GURL url_excluding_query = url.ReplaceComponents(replacements);
  const std::string& url_excluding_query_spec = url_excluding_query.spec();

  if (!RE2::FullMatch(url_excluding_query_spec,
                      search_engine->results_page_url_pattern)) {
    return false;
  }

  if (search_engine->search_term_query_key.empty()) {
    // Some search engines do not include the search term as a query parameter.
    // In that case, a match on `results_page_url_pattern` alone is enough.
    return true;
  }

  return net::GetValueForKeyInQuery(url, search_engine->search_term_query_key,
                                    /*out_value=*/nullptr);
}

std::optional<SearchEngineInfo> MaybeGetSearchEngine(const GURL& url) {
  if (!url.is_valid()) {
    return std::nullopt;
  }

  if (!url.SchemeIsHTTPOrHTTPS()) {
    return std::nullopt;
  }

  auto iter = kSearchEngines.find(url.host());
  if (iter == kSearchEngines.cend()) {
    return std::nullopt;
  }
  return iter->second;
}

}  // namespace metrics
