/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/search_engine/search_providers.h"

#include "base/optional.h"
#include "net/base/url_util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"
#include "bat/ads/internal/url_util.h"

namespace ads {

namespace {

base::Optional<SearchProviderInfo> GetSearchProviderForUrl(
    const std::string& url) {
  if (!GURL(url).is_valid()) {
    return base::nullopt;
  }

  for (const auto& search_provider : _search_providers) {
    if (!GURL(search_provider.hostname).is_valid()) {
      continue;
    }

    if (!SameDomainOrHost(url, search_provider.hostname)) {
      continue;
    }

    return search_provider;
  }

  return base::nullopt;
}

}  // namespace

bool IsSearchEngine(
    const std::string& url) {
  if (!GURL(url).is_valid()) {
    return false;
  }

  const base::Optional<SearchProviderInfo> search_provider =
      GetSearchProviderForUrl(url);
  if (!search_provider) {
    return false;
  }

  if (search_provider->is_always_classed_as_a_search) {
    return true;
  }

  const size_t index = search_provider->search_template.find('{');
  const std::string substring =
      search_provider->search_template.substr(0, index);
  const size_t href_index = url.find(substring);

  if (index != std::string::npos && href_index != std::string::npos) {
    return true;
  }

  return false;
}

bool IsSearchEngineResultsPages(
    const std::string& url) {
  if (!GURL(url).is_valid()) {
    return false;
  }

  const base::Optional<SearchProviderInfo> search_provider =
      GetSearchProviderForUrl(url);
  if (!search_provider) {
    return false;
  }

  const size_t index = search_provider->search_template.find('{');
  const std::string substring =
      search_provider->search_template.substr(0, index);
  const size_t href_index = url.find(substring);

  if (index != std::string::npos && href_index != std::string::npos) {
    return true;
  }

  return false;
}

std::string ExtractSearchQueryKeywords(
    const std::string& url) {
  std::string search_query_keywords;

  if (!GURL(url).is_valid()) {
    return search_query_keywords;
  }

  if (!IsSearchEngine(url)) {
    return search_query_keywords;
  }

  const base::Optional<SearchProviderInfo> search_provider =
      GetSearchProviderForUrl(url);
  if (!search_provider) {
    return search_query_keywords;
  }

  // Check if search template matches a search provider e.g.
  // https://searx.me/?q={searchTerms}&categories=general matches ?q={
  std::string key;
  if (!RE2::PartialMatch(search_provider->search_template,
      "\\?(.*?)\\={", &key)) {
    return search_query_keywords;
  }

  net::GetValueForKeyInQuery(GURL(url), key, &search_query_keywords);

  return search_query_keywords;
}

}  // namespace ads
