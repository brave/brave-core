/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/search_engine_util.h"

#include <vector>

#include "bat/ads/internal/base/search_engine_info.h"
#include "net/base/url_util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace ads {

namespace {

const std::vector<SearchEngineInfo> kSearchEngines = {
    SearchEngineInfo("Amazon",
                     "https://amazon.com",
                     "https://www.amazon.com/exec/obidos/external-search/"
                     "?field-keywords={searchTerms}&mode=blended",
                     false),
    SearchEngineInfo("Baidu",
                     "https://baidu.com",
                     "https://www.baidu.com/s?wd={searchTerms}",
                     true),
    SearchEngineInfo("Bing",
                     "https://bing.com",
                     "https://www.bing.com/search?q={searchTerms}",
                     true),
    SearchEngineInfo("DuckDuckGo",
                     "https://duckduckgo.com",
                     "https://duckduckgo.com/?q={searchTerms}&t=brave",
                     true),
    SearchEngineInfo("Fireball",
                     "https://fireball.com",
                     "https://fireball.com/search?q={searchTerms}",
                     true),
    SearchEngineInfo("GitHub",
                     "https://github.com",
                     "https://github.com/search?q={searchTerms}",
                     false),
    SearchEngineInfo(
        "Google",
        // TODO(https://github.com/brave/brave-browser/issues/8487): Brave Ads
        // search engines definition doesn't match all patterns
        "https://google.com",
        "https://www.google.com/search?q={searchTerms}",
        true),
    SearchEngineInfo("Google Japan",
                     "https://google.co.jp",
                     "https://www.google.co.jp/search?q={searchTerms}",
                     true),
    SearchEngineInfo("Stack Overflow",
                     "https://stackoverflow.com",
                     "https://stackoverflow.com/search?q={searchTerms}",
                     false),
    SearchEngineInfo("MDN Web Docs",
                     "https://developer.mozilla.org",
                     "https://developer.mozilla.org/search?q={searchTerms}",
                     false),
    SearchEngineInfo(
        "Twitter",
        "https://twitter.com",
        "https://twitter.com/search?q={searchTerms}&source=desktop-search",
        false),
    SearchEngineInfo(
        "Wikipedia",
        "https://en.wikipedia.org",
        "https://en.wikipedia.org/wiki/Special:Search?search={searchTerms}",
        false),
    SearchEngineInfo(
        "Yahoo",
        // TODO(https://github.com/brave/brave-browser/issues/8487): Brave Ads
        // search engines definition doesn't match all patterns
        "https://search.yahoo.com",
        "https://search.yahoo.com/search?p={searchTerms}&fr=opensearch",
        true),
    SearchEngineInfo(
        "Yahoo Japan",
        "https://search.yahoo.co.jp",
        "https://search.yahoo.co.jp/search?p={searchTerms}&fr=opensearch",
        true),
    SearchEngineInfo(
        "YouTube",
        "https://youtube.com",
        "https://www.youtube.com/"
        "results?search_type=search_videos&search_query={searchTerms}&search_"
        "sort=relevance&search_category=0&page=",
        false),
    SearchEngineInfo(
        "StartPage",
        // TODO(https://github.com/brave/brave-browser/issues/8487): Brave Ads
        // search engines definition doesn't match all patterns
        "https://startpage.com",
        "https://www.startpage.com/do/"
        "dsearch?query={searchTerms}&cat=web&pl=opensearch",
        true),
    SearchEngineInfo("Infogalactic",
                     "https://infogalactic.com",
                     "https://infogalactic.com/w/"
                     "index.php?title=Special:Search&search={searchTerms}",
                     false),
    SearchEngineInfo("Wolfram Alpha",
                     "https://wolframalpha.com",
                     "https://www.wolframalpha.com/input/?i={searchTerms}",
                     false),
    SearchEngineInfo("Semantic Scholar",
                     "https://semanticscholar.org",
                     "https://www.semanticscholar.org/search?q={searchTerms}",
                     true),
    SearchEngineInfo("Qwant",
                     "https://qwant.com",
                     "https://www.qwant.com/?q={searchTerms}&client=brave",
                     true),
    SearchEngineInfo(
        "Yandex",
        "https://yandex.com",
        "https://yandex.com/search/?text={searchTerms}&clid=2274777",
        true),
    SearchEngineInfo("Ecosia",
                     "https://ecosia.org",
                     "https://www.ecosia.org/search?q={searchTerms}",
                     true),
    SearchEngineInfo("searx",
                     "https://searx.me",
                     "https://searx.me/?q={searchTerms}&categories=general",
                     true),
    SearchEngineInfo("findx",
                     "https://findx.com",
                     "https://www.findx.com/search?q={searchTerms}&type=web",
                     true),
    SearchEngineInfo("Brave",
                     "https://search.brave.com/",
                     "https://search.brave.com/search?q={searchTerms}",
                     true)};

}  // namespace

bool IsSearchEngine(const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }

  bool is_a_search = false;

  for (const auto& search_engine : kSearchEngines) {
    const GURL hostname = GURL(search_engine.hostname);
    if (!hostname.is_valid()) {
      continue;
    }

    if (search_engine.is_always_classed_as_a_search &&
        url.DomainIs(hostname.host_piece())) {
      is_a_search = true;
      break;
    }

    size_t index = search_engine.query.find('{');
    std::string substring = search_engine.query.substr(0, index);
    size_t href_index = url.spec().find(substring);

    if (index != std::string::npos && href_index != std::string::npos) {
      is_a_search = true;
      break;
    }
  }

  return is_a_search;
}

std::string ExtractSearchQueryKeywords(const GURL& url) {
  std::string search_query_keywords;

  if (!IsSearchEngine(url)) {
    return search_query_keywords;
  }

  if (!url.is_valid()) {
    return search_query_keywords;
  }

  for (const auto& search_engine : kSearchEngines) {
    GURL search_engine_hostname = GURL(search_engine.hostname);
    if (!search_engine_hostname.is_valid()) {
      continue;
    }

    if (!url.DomainIs(search_engine_hostname.host_piece())) {
      continue;
    }

    // Checking if search query in as defined in |search_engine_util.h| is
    // defined, e.g. |https://searx.me/?q={searchTerms}&categories=general|
    // matches |?q={|
    std::string key;
    if (!RE2::PartialMatch(search_engine.query, "\\?(.*?)\\={", &key)) {
      return search_query_keywords;
    }

    net::GetValueForKeyInQuery(url, key, &search_query_keywords);
    break;
  }

  return search_query_keywords;
}

}  // namespace ads
