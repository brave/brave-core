/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/search_engine/search_engine_util.h"

#include <string>
#include <vector>

#include "base/strings/strcat.h"
#include "bat/ads/internal/common/search_engine/search_engine_domain_extension_constants.h"
#include "bat/ads/internal/common/search_engine/search_engine_subdomain_constants.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsSearchEngineUtilTest, IsMultilingualAmazonSearchEngine) {
  // Arrange
  const auto domain_extensions = GetAmazonSearchEngineDomainExtensions();

  // Act
  for (const auto& domain_extension : domain_extensions) {
    const GURL url =
        GURL(base::StrCat({"https://www.amazon.", domain_extension, "/"}));
    EXPECT_TRUE(IsSearchEngine(url));
  }

  // Assert
}

TEST(BatAdsSearchEngineUtilTest, IsNotMultilingualAmazonSearchEngine) {
  // Arrange

  // Act
  EXPECT_FALSE(IsSearchEngine(GURL("https://www.amazon.foobar/")));

  // Assert
}

TEST(BatAdsSearchEngineUtilTest, IsMultilingualGoogleSearchEngine) {
  // Arrange
  const auto domain_extensions = GetGoogleSearchEngineDomainExtensions();

  // Act
  for (const auto& domain_extension : domain_extensions) {
    const GURL url =
        GURL(base::StrCat({"https://www.google.", domain_extension, "/"}));
    EXPECT_TRUE(IsSearchEngine(url));
  }

  // Assert
}

TEST(BatAdsSearchEngineUtilTest, IsNotMultilingualGoogleSearchEngine) {
  // Arrange

  // Act
  EXPECT_FALSE(IsSearchEngine(GURL("https://www.google.foobar/")));

  // Assert
}

TEST(BatAdsSearchEngineUtilTest, IsMultilingualMojeekSearchEngine) {
  // Arrange
  const auto domain_extensions = GetMojeekSearchEngineDomainExtensions();

  // Act
  for (const auto& domain_extension : domain_extensions) {
    const GURL url =
        GURL(base::StrCat({"https://www.mojeek.", domain_extension, "/"}));
    EXPECT_TRUE(IsSearchEngine(url));
  }

  // Assert
}

TEST(BatAdsSearchEngineUtilTest, IsNotMultilingualMojeekSearchEngine) {
  // Arrange

  // Act
  EXPECT_FALSE(IsSearchEngine(GURL("https://www.mojeek.foobar/")));

  // Assert
}

TEST(BatAdsSearchEngineUtilTest, IsMultilingualWikipediaSearchEngine) {
  // Arrange
  const auto subdomains = GetWikipediaSearchEngineSubdomains();

  // Act
  for (const auto& subdomain : subdomains) {
    const GURL url =
        GURL(base::StrCat({"https://", subdomain, ".wikipedia.org/"}));
    EXPECT_TRUE(IsSearchEngine(url));
  }

  // Assert
}

TEST(BatAdsSearchEngineUtilTest, IsNotMultilingualWikipediaSearchEngine) {
  // Arrange

  // Act
  EXPECT_FALSE(IsSearchEngine(GURL("https://foobar.wikipedia.org/")));

  // Assert
}

TEST(BatAdsSearchEngineUtilTest, IsMultilingualYahooSearchEngine) {
  // Arrange
  const auto subdomains = GetYahooSearchEngineSubdomains();

  // Act
  for (const auto& subdomain : subdomains) {
    const GURL url =
        GURL(base::StrCat({"https://", subdomain, ".search.yahoo.com/"}));
    EXPECT_TRUE(IsSearchEngine(url));
  }

  // Assert
}

TEST(BatAdsSearchEngineUtilTest, IsNotMultilingualYahooSearchEngine) {
  // Arrange

  // Act
  EXPECT_FALSE(IsSearchEngine(GURL("https://foobar.search.yahoo.com/")));

  // Assert
}

TEST(BatAdsSearchEngineUtilTest, IsMonolingualSearchEngine) {
  // Arrange
  const std::vector<GURL> urls = {GURL("https://developer.mozilla.org/en-US/"),
                                  GURL("https://duckduckgo.com/"),
                                  GURL("https://fireball.de/"),
                                  GURL("https://github.com/"),
                                  GURL("https://infogalactic.com/"),
                                  GURL("https://search.brave.com/"),
                                  GURL("https://search.yahoo.com/"),
                                  GURL("https://stackoverflow.com/"),
                                  GURL("https://swisscows.com/"),
                                  GURL("https://twitter.com/explore/"),
                                  GURL("https://www.baidu.com/"),
                                  GURL("https://www.bing.com/"),
                                  GURL("https://www.dogpile.com/"),
                                  GURL("https://www.ecosia.org/"),
                                  GURL("https://www.excite.com/"),
                                  GURL("https://www.findx.com/"),
                                  GURL("https://www.gigablast.com/"),
                                  GURL("https://www.lycos.com/"),
                                  GURL("https://www.metacrawler.com/"),
                                  GURL("https://www.petalsearch.com/"),
                                  GURL("https://www.qwant.com/"),
                                  GURL("https://www.semanticscholar.org/"),
                                  GURL("https://www.sogou.com/"),
                                  GURL("https://www.startpage.com/"),
                                  GURL("https://www.webcrawler.com/"),
                                  GURL("https://www.wolframalpha.com/"),
                                  GURL("https://www.youtube.com/"),
                                  GURL("https://yandex.com/")};

  // Act
  for (const auto& url : urls) {
    EXPECT_TRUE(IsSearchEngine(url));
  }

  // Assert
}

TEST(BatAdsSearchEngineUtilTest, IsNotSearchEngine) {
  // Arrange
  const GURL url = GURL("https://foobar.com/");

  // Act
  const bool is_search_engine = IsSearchEngine(url);

  // Assert
  EXPECT_FALSE(is_search_engine);
}

TEST(BatAdsSearchEngineUtilTest, IsNotSearchEngineWithInvalidUrl) {
  // Arrange
  const GURL url = GURL("INVALID_URL");

  // Act
  const bool is_search_engine = IsSearchEngine(url);

  // Assert
  EXPECT_FALSE(is_search_engine);
}

}  // namespace ads
