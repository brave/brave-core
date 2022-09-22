/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/search_engine/search_engine_util.h"

#include <string>
#include <vector>

#include "base/strings/strcat.h"
#include "bat/ads/internal/base/search_engine/search_engine_domain_extension_constants.h"
#include "bat/ads/internal/base/search_engine/search_engine_subdomain_constants.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsSearchEngineUtilTest, IsMultilingualAmazonSearchEngine) {
  // Arrange
  const std::vector<std::string>& domain_extensions =
      GetAmazonSearchEngineDomainExtensions();

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
  const std::vector<std::string>& domain_extensions =
      GetGoogleSearchEngineDomainExtensions();

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
  const std::vector<std::string>& domain_extensions =
      GetMojeekSearchEngineDomainExtensions();

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
  const std::vector<std::string>& subdomains =
      GetWikipediaSearchEngineSubdomains();

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
  const std::vector<std::string>& subdomains = GetYahooSearchEngineSubdomains();

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
  const std::vector<GURL> urls = {
      GURL(R"(https://developer.mozilla.org/en-US/)"),
      GURL(R"(https://duckduckgo.com/)"),
      GURL(R"(https://fireball.de/)"),
      GURL(R"(https://github.com/)"),
      GURL(R"(https://infogalactic.com/)"),
      GURL(R"(https://search.brave.com/)"),
      GURL(R"(https://search.yahoo.com/)"),
      GURL(R"(https://stackoverflow.com/)"),
      GURL(R"(https://swisscows.com/)"),
      GURL(R"(https://twitter.com/explore/)"),
      GURL(R"(https://www.baidu.com/)"),
      GURL(R"(https://www.bing.com/)"),
      GURL(R"(https://www.dogpile.com/)"),
      GURL(R"(https://www.ecosia.org/)"),
      GURL(R"(https://www.excite.com/)"),
      GURL(R"(https://www.findx.com/)"),
      GURL(R"(https://www.gigablast.com/)"),
      GURL(R"(https://www.lycos.com/)"),
      GURL(R"(https://www.metacrawler.com/)"),
      GURL(R"(https://www.petalsearch.com/)"),
      GURL(R"(https://www.qwant.com/)"),
      GURL(R"(https://www.semanticscholar.org/)"),
      GURL(R"(https://www.sogou.com/)"),
      GURL(R"(https://www.startpage.com/)"),
      GURL(R"(https://www.webcrawler.com/)"),
      GURL(R"(https://www.wolframalpha.com/)"),
      GURL(R"(https://www.youtube.com/)"),
      GURL(R"(https://yandex.com/)")};

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
