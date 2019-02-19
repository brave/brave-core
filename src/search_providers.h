/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_SEARCH_PROVIDERS_H_
#define BAT_ADS_SEARCH_PROVIDERS_H_

#include <string>
#include <vector>

#include "search_provider_info.h"
#include "bat/ads/url_components.h"

namespace ads {

static const std::vector<SearchProviderInfo> _search_providers = {
  SearchProviderInfo(
      "Amazon",
      "www.amazon.com",
      "https://www.amazon.com/exec/obidos/external-search/"
      "?field-keywords={searchTerms}&mode=blended",
      false),
  SearchProviderInfo(
      "Bing",
      "www.bing.com",
      "https://www.bing.com/search?q={searchTerms}",
      true),
  SearchProviderInfo(
      "DuckDuckGo",
      "duckduckgo.com",
      "https://duckduckgo.com/?q={searchTerms}&t=brave",
      true),
  SearchProviderInfo(
      "Fireball",
      "fireball.com",
      "https://fireball.com/?q={searchTerms}",
      true),
  SearchProviderInfo(
      "GitHub",
      "github.com/search",
      "https://github.com/search?q={searchTerms}",
      false),
  SearchProviderInfo(
      "Google",
      "www.google.com",
      "https://www.google.com/search?q={searchTerms}",
      true),
  SearchProviderInfo(
      "Stack Overflow",
      "stackoverflow.com/search",
      "https://stackoverflow.com/search?q={searchTerms}",
      false),
  SearchProviderInfo(
      "MDN Web Docs",
      "developer.mozilla.org/search",
      "https://developer.mozilla.org/search?q={searchTerms}",
      false),
  SearchProviderInfo(
      "Twitter",
      "twitter.com",
      "https://twitter.com/search?q={searchTerms}&source=desktop-search",
      false),
  SearchProviderInfo(
      "Wikipedia",
      "en.wikipedia.org",
      "https://en.wikipedia.org/wiki/Special:Search?search={searchTerms}",
      false),
  SearchProviderInfo(
      "Yahoo",
      "search.yahoo.com",
      "https://search.yahoo.com/search?p={searchTerms}&fr=opensearch",
      true),
  SearchProviderInfo(
      "YouTube",
      "www.youtube.com",
      "https://www.youtube.com/results?search_type=search_videos&search_"
      "query={searchTerms}&search_sort=relevance&search_category=0&page=",
      false),
  SearchProviderInfo(
      "StartPage",
      "www.startpage.com",
      "https://www.startpage.com/do/dsearch?"
      "query={searchTerms}&cat=web&pl=opensearch",
      true),
  SearchProviderInfo(
      "Infogalactic",
      "infogalactic.com",
      "https://infogalactic.com/w/index.php?title="
      "Special:Search&search={searchTerms}",
      false),
  SearchProviderInfo(
      "Wolfram Alpha",
      "www.wolframalpha.com",
      "https://www.wolframalpha.com/input/?i={searchTerms}",
      false),
  SearchProviderInfo(
      "Semantic Scholar",
      "www.semanticscholar.org",
      "https://www.semanticscholar.org/search?q={searchTerms}",
      true),
  SearchProviderInfo(
      "Qwant",
      "www.qwant.com/",
      "https://www.qwant.com/?q={searchTerms}&client=brave",
      true),
  SearchProviderInfo(
      "Yandex",
      "yandex.com",
      "https://yandex.com/search/?text={searchTerms}&clid=2274777",
      true),
  SearchProviderInfo(
      "Ecosia",
      "www.ecosia.org/",
      "https://www.ecosia.org/search?q={searchTerms}",
      true),
  SearchProviderInfo(
      "searx",
      "searx.me",
      "https://searx.me/?q={searchTerms}&categories=general",
      true),
  SearchProviderInfo(
      "findx",
      "www.findx.com",
      "https://www.findx.com/search?q={searchTerms}&type=web",
      true)
};

class SearchProviders {
 public:
  SearchProviders();
  ~SearchProviders();

  static bool IsSearchEngine(const UrlComponents& components);
};

}  // namespace ads

#endif  // BAT_ADS_SEARCH_PROVIDERS_H_
