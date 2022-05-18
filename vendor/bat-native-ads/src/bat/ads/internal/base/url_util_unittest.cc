/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/url_util.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsUrlUtilTest, UrlMatchesPatternWithNoWildcards) {
  // Arrange
  const GURL url = GURL("https://www.foo.com/");
  const std::string pattern = "https://www.foo.com/";

  // Act
  const bool does_match = DoesUrlMatchPattern(url, pattern);

  // Assert
  EXPECT_TRUE(does_match);
}

TEST(BatAdsUrlUtilTest, UrlWithPathMatchesPatternWithNoWildcards) {
  // Arrange
  const GURL url = GURL("https://www.foo.com/bar");
  const std::string pattern = "https://www.foo.com/bar";

  // Act
  const bool does_match = DoesUrlMatchPattern(url, pattern);

  // Assert
  EXPECT_TRUE(does_match);
}

TEST(BatAdsUrlUtilTest, UrlDoesNotMatchPattern) {
  // Arrange
  const GURL url = GURL("https://www.foo.com/");
  const std::string pattern = "www.foo.com";

  // Act
  const bool does_match = DoesUrlMatchPattern(url, pattern);

  // Assert
  EXPECT_FALSE(does_match);
}

TEST(BatAdsUrlUtilTest, UrlDoesNotMatchPatternWithMissingEmptyPath) {
  // Arrange
  const GURL url = GURL("https://www.foo.com/");
  const std::string pattern = "https://www.foo.com";

  // Act
  const bool does_match = DoesUrlMatchPattern(url, pattern);

  // Assert
  EXPECT_FALSE(does_match);
}

TEST(BatAdsUrlUtilTest, UrlMatchesEndWildcardPattern) {
  // Arrange
  const GURL url = GURL("https://www.foo.com/bar?key=test");
  const std::string pattern = "https://www.foo.com/bar*";

  // Act
  const bool does_match = DoesUrlMatchPattern(url, pattern);

  // Assert
  EXPECT_TRUE(does_match);
}

TEST(BatAdsUrlUtilTest, UrlMatchesMidWildcardPattern) {
  // Arrange
  const GURL url = GURL("https://www.foo.com/woo-bar-hoo");
  const std::string pattern = "https://www.foo.com/woo*hoo";

  // Act
  const bool does_match = DoesUrlMatchPattern(url, pattern);

  // Assert
  EXPECT_TRUE(does_match);
}

TEST(BatAdsUrlUtilTest, UrlDoesNotMatchMidWildcardPattern) {
  // Arrange
  const GURL url = GURL("https://www.foo.com/woo");
  const std::string pattern = "https://www.foo.com/woo*hoo";

  // Act
  const bool does_match = DoesUrlMatchPattern(url, pattern);

  // Assert
  EXPECT_FALSE(does_match);
}

TEST(BatAdsUrlUtilTest, SameDomainOrHost) {
  // Arrange
  const GURL url1 = GURL("https://foo.com?bar=test");
  const GURL url2 = GURL("https://subdomain.foo.com/bar");

  // Act
  const bool is_same_site = SameDomainOrHost(url1, url2);

  // Assert
  EXPECT_TRUE(is_same_site);
}

TEST(BatAdsUrlUtilTest, NotSameDomainOrHost) {
  // Arrange
  const GURL url1 = GURL("https://foo.com?bar=test");
  const GURL url2 = GURL("https://subdomain.bar.com/foo");

  // Act
  const bool is_same_site = SameDomainOrHost(url1, url2);

  // Assert
  EXPECT_FALSE(is_same_site);
}

TEST(BatAdsUrlUtilTest, SameDomainOrHostForUrlWithNoSubdomain) {
  // Arrange
  const GURL url1 = GURL("https://foo.com?bar=test");
  const GURL url2 = GURL("https://foo.com/bar");

  // Act
  const bool is_same_site = SameDomainOrHost(url1, url2);

  // Assert
  EXPECT_TRUE(is_same_site);
}

TEST(BatAdsUrlUtilTest, NotSameDomainOrHostForUrlWithNoSubdomain) {
  // Arrange
  const GURL url1 = GURL("https://foo.com?bar=test");
  const GURL url2 = GURL("https://bar.com/foo");

  // Act
  const bool is_same_site = SameDomainOrHost(url1, url2);

  // Assert
  EXPECT_FALSE(is_same_site);
}

TEST(BatAdsUrlUtilTest, SameDomainOrHostForUrlWithRef) {
  // Arrange
  const GURL url1 = GURL("https://foo.com?bar=test#ref");
  const GURL url2 = GURL("https://foo.com/bar");

  // Act
  const bool is_same_site = SameDomainOrHost(url1, url2);

  // Assert
  EXPECT_TRUE(is_same_site);
}

TEST(BatAdsUrlUtilTest, NotSameDomainOrHostForUrlWithRef) {
  // Arrange
  const GURL url1 = GURL("https://foo.com?bar=test#ref");
  const GURL url2 = GURL("https://bar.com/foo");

  // Act
  const bool is_same_site = SameDomainOrHost(url1, url2);

  // Assert
  EXPECT_FALSE(is_same_site);
}

TEST(BatAdsUrlUtilTest, DomainOrHostExists) {
  // Arrange
  const std::vector<GURL> urls = {GURL("https://foo.com"),
                                  GURL("https://bar.com")};

  const GURL url = GURL("https://bar.com/foo");

  // Act
  const bool does_exist = DomainOrHostExists(urls, url);

  // Assert
  EXPECT_TRUE(does_exist);
}

TEST(BatAdsUrlUtilTest, DomainOrHostDoesNotExist) {
  // Arrange
  const std::vector<GURL> urls = {GURL("https://foo.com"),
                                  GURL("https://bar.com")};

  const GURL url = GURL("https://baz.com/qux");

  // Act
  const bool does_exist = DomainOrHostExists(urls, url);

  // Assert
  EXPECT_FALSE(does_exist);
}

}  // namespace ads
