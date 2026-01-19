/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/search_engine_util.h"

#include <optional>
#include <string_view>

#include "brave/components/serp_metrics/search_engine_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace metrics {

namespace {

constexpr std::string_view kSearchEnginetUrls[] = {
    /* Brave */
    "https://search.brave.com/search?q=test",

    /* Google */
    "https://www.google.com/search?q=test",

    /* DuckDuckGo */
    "https://duckduckgo.com/?q=test",

    /* Qwant */
    "https://www.qwant.com/?q=test",

    /* Bing */
    "https://www.bing.com/search?q=test",

    /* Startpage */
    "https://www.startpage.com/sp/search",

    /* Ecosia */
    "https://www.ecosia.org/search?q=test",

    /* Ask */
    "https://www.ask.com/web?q=test",

    /* Baidu */
    "https://www.baidu.com/s?wd=test",

    /* ChatGPT */
    "https://chatgpt.com/c/test",

    /* CocCoc */
    "https://coccoc.com/search?query=test",

    /* Daum */
    "https://search.daum.net/search?q=test",

    /* Dogpile */
    "https://www.dogpile.com/serp?q=test",

    /* Excite */
    "https://results.excite.com/serp?q=test",

    /* Fireball */
    "https://fireball.com/search/?q=test",

    /* Freespoke */
    "https://freespoke.com/search/web?q=test",

    /* Info.com */
    "https://www.info.com/serp?q=test",

    /* Kagi */
    "https://kagi.com/search?q=test",

    /* Karma Search */
    "https://karmasearch.org/search?q=test",

    /* Lilo */
    "https://search.lilo.org/?q=test",

    /* Metacrawler */
    "https://www.metacrawler.com/serp?q=test",

    /* Mail.ru */
    "https://mail.ru/search?search_source=test",

    /* Mojeek */
    "https://www.mojeek.com/search?q=test",

    /* Naver */
    "https://search.naver.com/search.naver?query=test",

    /* Nona */
    "https://www.nona.de/?q=test",

    /* Perplexity */
    "https://www.perplexity.ai/search/test",

    /* PrivacyWall */
    "https://www.privacywall.org/search/secure?q=test",

    /* Quendu */
    "https://quendu.com/search?q=test",

    /* Seznam */
    "https://search.seznam.cz/?q=test",

    /* 360 Search */
    "https://www.so.com/s?q=test",

    /* Sogou */
    "https://www.sogou.com/web?query=test",

    /* WebCrawler */
    "https://www.webcrawler.com/serp?q=test",

    /* Yahoo */
    "https://search.yahoo.com/search?p=test",

    /* Yahoo! JAPAN */
    "https://search.yahoo.co.jp/search?p=test",

    /* Yandex */
    "https://yandex.com/search/?text=test",

    /* Yep */
    "https://yep.com/web?q=test",

    /* You.com */
    "https://you.com/search?q=test",
};

constexpr std::string_view kGenericUrls[] = {
    "https://foo.com/",
    "https://foo.com/search",
    "https://foo.com/docs/search/help",
    "https://foo.com/?q=test",
    "https://foo.com/page?p=123",
    "https://foo.com/view?text=hello",
    "https://foo.com/search?q=test",
    "https://foo.com/page#search",
    "https://foo.com:8443/search",
    "http://127.0.0.1/search",
    "https://www.foo.com/search",
    "https://blog.foo.com/search",
    "https://api.services.foo.com/search?q=test",
    "http://bar.org/search",
    "https://www.baz.net/?q=test",
    "https://search.qux.io/results",
    "https://qux.co.uk/search",
    "http://localhost/search",
};

}  // namespace

TEST(SerpMetricsSearchEngineUtilTest, IsSearchEngineResultsPage) {
  for (std::string_view url_string : kSearchEnginetUrls) {
    GURL url(url_string);
    ASSERT_TRUE(url.is_valid()) << url;
    EXPECT_TRUE(IsSearchEngineResultsPage(url)) << url;
  }
}

TEST(SerpMetricsSearchEngineUtilTest, IsNotSearchEngineResultsPage) {
  for (std::string_view url_string : kGenericUrls) {
    GURL url(url_string);
    ASSERT_TRUE(url.is_valid()) << url;
    EXPECT_FALSE(IsSearchEngineResultsPage(url)) << url;
  }
}

TEST(SerpMetricsSearchEngineUtilTest, GetSearchEngine) {
  for (std::string_view url_string : kSearchEnginetUrls) {
    GURL url(url_string);
    ASSERT_TRUE(url.is_valid()) << url;
    std::optional<SearchEngineInfo> search_engine = MaybeGetSearchEngine(url);
    EXPECT_TRUE(search_engine) << url;
  }
}

TEST(SerpMetricsSearchEngineUtilTest, DoNotGetSearchEngine) {
  for (std::string_view url_string : kGenericUrls) {
    GURL url(url_string);
    ASSERT_TRUE(url.is_valid()) << url;
    std::optional<SearchEngineInfo> search_engine = MaybeGetSearchEngine(url);
    EXPECT_FALSE(search_engine) << url;
  }
}

}  // namespace metrics
