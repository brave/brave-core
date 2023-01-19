/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/search_engine/search_engine_results_page_util.h"

#include <vector>

#include "bat/ads/internal/common/search_engine/search_engine_constants.h"
#include "bat/ads/internal/common/search_engine/search_engine_info.h"
#include "bat/ads/internal/common/url/url_util.h"
#include "net/base/url_util.h"  // IWYU pragma: keep
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace ads {

namespace {

absl::optional<SearchEngineInfo> FindSearchEngineResultsPage(const GURL& url) {
  if (!url.is_valid()) {
    return absl::nullopt;
  }

  const GURL url_with_empty_query = GetUrlWithEmptyQuery(url);
  const std::vector<SearchEngineInfo>& search_engines = GetSearchEngines();
  for (const auto& search_engine : search_engines) {
    if (RE2::FullMatch(url_with_empty_query.spec(),
                       search_engine.result_page_url_pattern)) {
      return search_engine;
    }
  }

  return absl::nullopt;
}

}  // namespace

bool IsSearchEngineResultsPage(const GURL& url) {
  const absl::optional<SearchEngineInfo> search_engine =
      FindSearchEngineResultsPage(url);
  if (!search_engine) {
    return false;
  }

  if (search_engine->search_term_query_key.empty()) {
    // We should only match |result_page_url_pattern| if the search engine does
    // not have a search term query key
    return true;
  }

  std::string search_term_query_value;
  return net::GetValueForKeyInQuery(url, search_engine->search_term_query_key,
                                    &search_term_query_value);
}

absl::optional<std::string> ExtractSearchTermQueryValue(const GURL& url) {
  const absl::optional<SearchEngineInfo> search_engine =
      FindSearchEngineResultsPage(url);
  if (!search_engine) {
    return absl::nullopt;
  }

  std::string search_term_query_value;
  if (!net::GetValueForKeyInQuery(url, search_engine->search_term_query_key,
                                  &search_term_query_value)) {
    return absl::nullopt;
  }

  return search_term_query_value;
}

}  // namespace ads
