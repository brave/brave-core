/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/search_engine/search_providers.h"

#include "net/base/url_util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace ads {

SearchProviders::SearchProviders() = default;

SearchProviders::~SearchProviders() = default;

bool SearchProviders::IsSearchEngine(
    const std::string& url) {
  const GURL visited_url = GURL(url);
  if (!visited_url.is_valid()) {
    return false;
  }

  bool is_a_search = false;

  for (const auto& search_provider : _search_providers) {
    const GURL search_provider_hostname = GURL(search_provider.hostname);
    if (!search_provider_hostname.is_valid()) {
      continue;
    }

    if (search_provider.is_always_classed_as_a_search &&
        visited_url.DomainIs(search_provider_hostname.host_piece())) {
      is_a_search = true;
      break;
    }

    size_t index = search_provider.search_template.find('{');
    std::string substring = search_provider.search_template.substr(0, index);
    size_t href_index = url.find(substring);

    if (index != std::string::npos && href_index != std::string::npos) {
      is_a_search = true;
      break;
    }
  }

  return is_a_search;
}

std::string SearchProviders::ExtractSearchQueryKeywords(
    const std::string& url) {
  std::string search_query_keywords = "";

  const GURL visited_url = GURL(url);
  if (!visited_url.is_valid()) {
    return search_query_keywords;
  }

  for (const auto& search_provider : _search_providers) {
    GURL search_provider_hostname = GURL(search_provider.hostname);
    if (!search_provider_hostname.is_valid()) {
      continue;
    }

    if (!visited_url.DomainIs(search_provider_hostname.host_piece())) {
      continue;
    }

    // Checking if search template in as defined in |search_providers.h|
    // is defined, e.g. |https://searx.me/?q={searchTerms}&categories=general|
    // matches |?q={|
    std::string key;
    if (!RE2::PartialMatch(
        search_provider.search_template, "\\?(.*?)\\={", &key)) {
      return search_query_keywords;
    }

    net::GetValueForKeyInQuery(GURL(url), key, &search_query_keywords);
    break;
  }

  return search_query_keywords;
}

}  // namespace ads
