/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SEARCH_PROVIDERS_H_
#define BAT_ADS_INTERNAL_SEARCH_PROVIDERS_H_

#include <string>
#include <vector>

#include "bat/ads/internal/search_provider_info.h"

namespace ads {

static const std::vector<SearchProviderInfo> _search_providers = {
  SearchProviderInfo(
      "Amazon",
      "https://amazon.com",
      "https://www.amazon.com/exec/obidos/external-search/?field-keywords={searchTerms}&mode=blended",  // NOLINT
      false),
  SearchProviderInfo(
      "Bing",
      "https://bing.com",
      "https://www.bing.com/search?q={searchTerms}",
      true),
  SearchProviderInfo(
      "DuckDuckGo",
      "https://duckduckgo.com",
      "https://duckduckgo.com/?q={searchTerms}&t=brave",
      true),
  SearchProviderInfo(
      "Fireball",
      "https://fireball.com",
      "https://fireball.com/?q={searchTerms}",
      true),
  SearchProviderInfo(
      "GitHub",
      "https://github.com",
      "https://github.com/search?q={searchTerms}",
      false),
  SearchProviderInfo(
      "Google",
      "https://google.com",
      "https://www.google.com/search?q={searchTerms}",
      true),
  SearchProviderInfo(
      "Stack Overflow",
      "https://stackoverflow.com",
      "https://stackoverflow.com/search?q={searchTerms}",
      false),
  SearchProviderInfo(
      "MDN Web Docs",
      "https://developer.mozilla.org",
      "https://developer.mozilla.org/search?q={searchTerms}",
      false),
  SearchProviderInfo(
      "Twitter",
      "https://twitter.com",
      "https://twitter.com/search?q={searchTerms}&source=desktop-search",
      false),
  SearchProviderInfo(
      "Wikipedia",
      "https://en.wikipedia.org",
      "https://en.wikipedia.org/wiki/Special:Search?search={searchTerms}",
      false),
  SearchProviderInfo(
      "Yahoo",
      "https://search.yahoo.com",
      "https://search.yahoo.com/search?p={searchTerms}&fr=opensearch",
      true),
  SearchProviderInfo(
      "YouTube",
      "https://youtube.com",
      "https://www.youtube.com/results?search_type=search_videos&search_query={searchTerms}&search_sort=relevance&search_category=0&page=",  // NOLINT
      false),
  SearchProviderInfo(
      "StartPage",
      "https://startpage.com",
      "https://www.startpage.com/do/dsearch?query={searchTerms}&cat=web&pl=opensearch",  // NOLINT
      true),
  SearchProviderInfo(
      "Infogalactic",
      "https://infogalactic.com",
      "https://infogalactic.com/w/index.php?title=Special:Search&search={searchTerms}",  // NOLINT
      false),
  SearchProviderInfo(
      "Wolfram Alpha",
      "https://wolframalpha.com",
      "https://www.wolframalpha.com/input/?i={searchTerms}",
      false),
  SearchProviderInfo(
      "Semantic Scholar",
      "https://semanticscholar.org",
      "https://www.semanticscholar.org/search?q={searchTerms}",
      true),
  SearchProviderInfo(
      "Qwant",
      "https://qwant.com",
      "https://www.qwant.com/?q={searchTerms}&client=brave",
      true),
  SearchProviderInfo(
      "Yandex",
      "https://yandex.com",
      "https://yandex.com/search/?text={searchTerms}&clid=2274777",
      true),
  SearchProviderInfo(
      "Ecosia",
      "https://ecosia.org",
      "https://www.ecosia.org/search?q={searchTerms}",
      true),
  SearchProviderInfo(
      "searx",
      "https://searx.me",
      "https://searx.me/?q={searchTerms}&categories=general",
      true),
  SearchProviderInfo(
      "findx",
      "https://findx.com",
      "https://www.findx.com/search?q={searchTerms}&type=web",
      true)
};

class SearchProviders {
 public:
  SearchProviders();
  ~SearchProviders();

  static bool IsSearchEngine(const std::string& url);
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SEARCH_PROVIDERS_H_
