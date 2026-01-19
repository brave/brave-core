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

constexpr std::string_view kSearchEngineResultsPageUrls[] = {
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

constexpr std::string_view kSearchEngineNonResultsPageUrls[] = {
    /* Brave */
    "https://search.brave.com/",
    "https://search.brave.com/news",
    "https://search.brave.com/images",

    /* Google */
    "https://www.google.com/",
    "https://www.google.com/about",
    "https://www.google.com/preferences",
    "https://www.google.com/search",
    "https://www.google.com/search?hl=en",

    /* DuckDuckGo */
    "https://duckduckgo.com/",
    "https://duckduckgo.com/about",
    "https://duckduckgo.com/?ia=web",

    /* Qwant */
    "https://www.qwant.com/",
    "https://www.qwant.com/maps",
    "https://www.qwant.com/?lang=en",

    /* Bing */
    "https://www.bing.com/",
    "https://www.bing.com/account/general",
    "https://www.bing.com/search",
    "https://www.bing.com/search?cc=US",

    /* Startpage */
    "https://www.startpage.com/",
    "https://www.startpage.com/sp",

    /* Ecosia */
    "https://www.ecosia.org/",
    "https://www.ecosia.org/about",
    "https://www.ecosia.org/search",

    /* Ask */
    "https://www.ask.com/",
    "https://www.ask.com/web",

    /* Baidu */
    "https://www.baidu.com/",
    "https://www.baidu.com/somethingelse",
    "https://www.baidu.com/s",
    "https://www.baidu.com/s?ie=utf-8",
    "https://www.baidu.com/s?word=test",

    /* ChatGPT */
    "https://chatgpt.com/",
    "https://chatgpt.com/about",
    "https://chatgpt.com/c",

    /* CocCoc */
    "https://coccoc.com/",
    "https://coccoc.com/search",

    /* Daum */
    "https://search.daum.net/",
    "https://search.daum.net/search",

    /* Dogpile */
    "https://www.dogpile.com/",
    "https://www.dogpile.com/serp",

    /* Excite */
    "https://results.excite.com/",
    "https://results.excite.com/serp",

    /* Fireball */
    "https://fireball.com/",
    "https://fireball.com/search/",

    /* Freespoke */
    "https://freespoke.com/",
    "https://freespoke.com/search/web",

    /* Info.com */
    "https://www.info.com/",
    "https://www.info.com/serp",

    /* Kagi */
    "https://kagi.com/",
    "https://kagi.com/search",

    /* Karma Search */
    "https://karmasearch.org/",
    "https://karmasearch.org/search",

    /* Lilo */
    "https://search.lilo.org/",
    "https://search.lilo.org/?lang=en",

    /* Metacrawler */
    "https://www.metacrawler.com/",
    "https://www.metacrawler.com/serp",

    /* Mail.ru */
    "https://mail.ru/",
    "https://mail.ru/search",

    /* Mojeek */
    "https://www.mojeek.com/",
    "https://www.mojeek.com/search",

    /* Naver */
    "https://search.naver.com/",
    "https://search.naver.com/search.naver",

    /* Nona */
    "https://www.nona.de/",
    "https://www.nona.de/?lang=de",

    /* Perplexity */
    "https://www.perplexity.ai/",
    "https://www.perplexity.ai/search",
    "https://www.perplexity.ai/search/",

    /* PrivacyWall */
    "https://www.privacywall.org/",
    "https://www.privacywall.org/search/secure",

    /* Quendu */
    "https://quendu.com/",
    "https://quendu.com/search",

    /* Seznam */
    "https://search.seznam.cz/",
    "https://search.seznam.cz/?hl=cs",

    /* 360 Search */
    "https://www.so.com/",
    "https://www.so.com/s",

    /* Sogou */
    "https://www.sogou.com/",
    "https://www.sogou.com/web",

    /* WebCrawler */
    "https://www.webcrawler.com/",
    "https://www.webcrawler.com/serp",

    /* Yahoo */
    "https://search.yahoo.com/",
    "https://search.yahoo.com/search",

    /* Yahoo! JAPAN */
    "https://search.yahoo.co.jp/",
    "https://search.yahoo.co.jp/search",

    /* Yandex */
    "https://yandex.com/",
    "https://yandex.com/search/",

    /* Yep */
    "https://yep.com/",
    "https://yep.com/web",

    /* You.com */
    "https://you.com/",
    "https://you.com/search",
};

constexpr std::string_view kNonSearchEngineUrls[] = {
    "http://127.0.0.1/garply",
    "http://localhost/thud",
    "https://bar.com/search",
    "https://baz.com/docs/qux/help",
    "https://corge.com:8443/grault",
    "https://foo.com/",
    "https://fred.co.uk/search",
    "https://googleacom/search?q=test",
    "https://grault.com/page?p=garply",
    "https://metasyntactic.com/xyzzy",
    "https://metasyntactic.net/search?q=foo",
    "https://plugh.com/search?q=xyzzy",
    "https://quux.com/?q=corge",
    "https://search.metasyntactic.io/results",
    "https://thud.com/page#foo",
    "https://waldo.com/view?text=fred",
    "https://www.google-com/search?q=test",
    "https://www.googleXcom/search?q=test",
    "https://www.googleacom/search?q=test",
    "https://www.waldo.com/fred",
};

constexpr std::string_view kMalformedUrls[] = {
    "",
    "http:/",
    "search",
    "www.google.com",
    "https://",
    "ftp://search.brave.com/search?q=test",
    "https://.com/",
    "https://?q=test",
};

constexpr std::string_view kSpoofedSearchEngineHostUrls[] = {
    "https://www.google.com.evil.com/search?q=test",
    "https://duckduckgo.com.evil.com/?q=test",
    "https://www.bing.com.evil.com/search?q=test",
    "https://search.yahoo.com.evil.com/search?p=test",
    "https://chatgpt.com.evil.com/c/test",
    "https://www.perplexity.ai.evil.com/search/test",
};

}  // namespace

TEST(SerpMetricsSearchEngineUtilTest, DetectSearchEngineResultsPageUrls) {
  for (std::string_view url_string : kSearchEngineResultsPageUrls) {
    GURL url(url_string);
    EXPECT_TRUE(IsSearchEngineResultsPage(url)) << url;
  }
}

TEST(SerpMetricsSearchEngineUtilTest,
     DoNotDetectNonSearchEngineResultsPageUrls) {
  for (std::string_view url_string : kNonSearchEngineUrls) {
    GURL url(url_string);
    EXPECT_FALSE(IsSearchEngineResultsPage(url)) << url;
  }
}

TEST(SerpMetricsSearchEngineUtilTest, DetectSearchEngineUrls) {
  for (std::string_view url_string : kSearchEngineResultsPageUrls) {
    GURL url(url_string);
    std::optional<SearchEngineInfo> search_engine = MaybeGetSearchEngine(url);
    EXPECT_TRUE(search_engine) << url;
  }
}

TEST(SerpMetricsSearchEngineUtilTest, DoNotDetectNonSearchEngineUrls) {
  for (std::string_view url_string : kNonSearchEngineUrls) {
    GURL url(url_string);
    std::optional<SearchEngineInfo> search_engine = MaybeGetSearchEngine(url);
    EXPECT_FALSE(search_engine) << url;
  }
}

TEST(SerpMetricsSearchEngineUtilTest,
     DoNotDetectSearchEngineNonResultsPageUrls) {
  for (std::string_view url_string : kSearchEngineNonResultsPageUrls) {
    GURL url(url_string);
    EXPECT_FALSE(IsSearchEngineResultsPage(url)) << url;
  }
}

TEST(SerpMetricsSearchEngineUtilTest, DoNotDetectMalformedUrls) {
  for (std::string_view url_string : kMalformedUrls) {
    GURL url(url_string);
    EXPECT_FALSE(metrics::IsSearchEngineResultsPage(url)) << url;
    EXPECT_FALSE(metrics::MaybeGetSearchEngine(url)) << url;
  }
}

TEST(SerpMetricsSearchEngineUtilTest, DoNotDetectSpoofedSearchEngineHostUrls) {
  for (std::string_view url_string : kSpoofedSearchEngineHostUrls) {
    GURL url(url_string);
    EXPECT_FALSE(IsSearchEngineResultsPage(url)) << url;
    EXPECT_FALSE(MaybeGetSearchEngine(url)) << url;
  }
}

}  // namespace metrics
