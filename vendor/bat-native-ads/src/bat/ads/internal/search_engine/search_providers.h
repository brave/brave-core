/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SEARCH_ENGINE_SEARCH_PROVIDERS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SEARCH_ENGINE_SEARCH_PROVIDERS_H_

#include <string>
#include <vector>

#include "bat/ads/internal/search_engine/search_provider_info.h"

namespace ads {

const std::vector<SearchProviderInfo> _search_providers = {
    SearchProviderInfo("Amazon",
                       "https://amazon.com",
                       "https://www.amazon.com/exec/obidos/external-search/"
                       "?field-keywords={searchTerms}&mode=blended",
                       false),
    SearchProviderInfo("Baidu",
                       "https://baidu.com",
                       "https://www.baidu.com/s?wd={searchTerms}",
                       true),
    SearchProviderInfo("Bing",
                       "https://bing.com",
                       "https://www.bing.com/search?q={searchTerms}",
                       true),
    SearchProviderInfo("DuckDuckGo",
                       "https://duckduckgo.com",
                       "https://duckduckgo.com/?q={searchTerms}&t=brave",
                       true),
    SearchProviderInfo("Fireball",
                       "https://fireball.com",
                       "https://fireball.com/search?q={searchTerms}",
                       true),
    SearchProviderInfo("GitHub",
                       "https://github.com",
                       "https://github.com/search?q={searchTerms}",
                       false),
    SearchProviderInfo(
        "Google",
        // TODO(https://github.com/brave/brave-browser/issues/8487): Brave Ads
        // search providers definition doesn't match all patterns
        "https://google.com",
        "https://www.google.com/search?q={searchTerms}",
        true),
    SearchProviderInfo("Google Japan",
                       "https://google.co.jp",
                       "https://www.google.co.jp/search?q={searchTerms}",
                       true),
    SearchProviderInfo("Stack Overflow",
                       "https://stackoverflow.com",
                       "https://stackoverflow.com/search?q={searchTerms}",
                       false),
    SearchProviderInfo("MDN Web Docs",
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
        // TODO(https://github.com/brave/brave-browser/issues/8487): Brave Ads
        // search providers definition doesn't match all patterns
        "https://search.yahoo.com",
        "https://search.yahoo.com/search?p={searchTerms}&fr=opensearch",
        true),
    SearchProviderInfo(
        "Yahoo Japan",
        "https://search.yahoo.co.jp",
        "https://search.yahoo.co.jp/search?p={searchTerms}&fr=opensearch",
        true),
    SearchProviderInfo(
        "YouTube",
        "https://youtube.com",
        "https://www.youtube.com/"
        "results?search_type=search_videos&search_query={searchTerms}&search_"
        "sort=relevance&search_category=0&page=",
        false),
    SearchProviderInfo(
        "StartPage",
        // TODO(https://github.com/brave/brave-browser/issues/8487): Brave Ads
        // search providers definition doesn't match all patterns
        "https://startpage.com",
        "https://www.startpage.com/do/"
        "dsearch?query={searchTerms}&cat=web&pl=opensearch",
        true),
    SearchProviderInfo("Infogalactic",
                       "https://infogalactic.com",
                       "https://infogalactic.com/w/"
                       "index.php?title=Special:Search&search={searchTerms}",
                       false),
    SearchProviderInfo("Wolfram Alpha",
                       "https://wolframalpha.com",
                       "https://www.wolframalpha.com/input/?i={searchTerms}",
                       false),
    SearchProviderInfo("Semantic Scholar",
                       "https://semanticscholar.org",
                       "https://www.semanticscholar.org/search?q={searchTerms}",
                       true),
    SearchProviderInfo("Qwant",
                       "https://qwant.com",
                       "https://www.qwant.com/?q={searchTerms}&client=brave",
                       true),
    SearchProviderInfo(
        "Yandex",
        "https://yandex.com",
        "https://yandex.com/search/?text={searchTerms}&clid=2274777",
        true),
    SearchProviderInfo("Ecosia",
                       "https://ecosia.org",
                       "https://www.ecosia.org/search?q={searchTerms}",
                       true),
    SearchProviderInfo("searx",
                       "https://searx.me",
                       "https://searx.me/?q={searchTerms}&categories=general",
                       true),
    SearchProviderInfo("findx",
                       "https://findx.com",
                       "https://www.findx.com/search?q={searchTerms}&type=web",
                       true),
    SearchProviderInfo("Brave",
                       "https://search.brave.com/",
                       "https://search.brave.com/search?q={searchTerms}",
                       true)};

class SearchProviders {
 public:
  SearchProviders();
  ~SearchProviders();

  static bool IsSearchEngine(const std::string& url);
  static std::string ExtractSearchQueryKeywords(const std::string& url);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SEARCH_ENGINE_SEARCH_PROVIDERS_H_
