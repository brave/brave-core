/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_results_page_util.h"

#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_constants.h"
#include "brave/components/brave_ads/core/internal/common/search_engine/search_engine_info.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "net/base/url_util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

std::optional<SearchEngineInfo> FindSearchEngineResultsPage(const GURL& url) {
  if (!url.is_valid()) {
    return std::nullopt;
  }

  const std::string url_with_empty_query_spec =
      GetUrlWithEmptyQuery(url).spec();

  for (const auto& search_engine : GetSearchEngines()) {
    if (RE2::FullMatch(url_with_empty_query_spec,
                       search_engine.result_page_url_pattern)) {
      return search_engine;
    }
  }

  return std::nullopt;
}

}  // namespace

bool IsSearchEngineResultsPage(const GURL& url) {
  const std::optional<SearchEngineInfo> search_engine =
      FindSearchEngineResultsPage(url);
  if (!search_engine) {
    return false;
  }

  if (search_engine->search_term_query_key.empty()) {
    // We should only match `result_page_url_pattern` if the search engine does
    // not have a search term query key
    return true;
  }

  std::string search_term_query_value;
  return net::GetValueForKeyInQuery(url, search_engine->search_term_query_key,
                                    &search_term_query_value);
}

std::optional<std::string> ExtractSearchTermQueryValue(const GURL& url) {
  const std::optional<SearchEngineInfo> search_engine =
      FindSearchEngineResultsPage(url);
  if (!search_engine) {
    return std::nullopt;
  }

  std::string search_term_query_value;
  if (!net::GetValueForKeyInQuery(url, search_engine->search_term_query_key,
                                  &search_term_query_value)) {
    return std::nullopt;
  }

  return search_term_query_value;
}

}  // namespace brave_ads
