/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/url_util.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsUrlUtilTest,
    UrlMatchesPatternWithNoWildcards) {
  // Arrange
  const std::string url = "https://www.foo.com/";
  const std::string pattern = "https://www.foo.com/";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_TRUE(does_match);
}

TEST(BatAdsUrlUtilTest,
    UrlWithPathMatchesPatternWithNoWildcards) {
  // Arrange
  const std::string url = "https://www.foo.com/bar";
  const std::string pattern = "https://www.foo.com/bar";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_TRUE(does_match);
}

TEST(BatAdsUrlUtilTest,
    UrlDoesNotMatchPattern) {
  // Arrange
  const std::string url = "https://www.foo.com/";
  const std::string pattern = "www.foo.com";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_FALSE(does_match);
}

TEST(BatAdsUrlUtilTest,
    UrlDoesNotMatchPatternWithMissingEmptyPath) {
  // Arrange
  const std::string url = "https://www.foo.com/";
  const std::string pattern = "https://www.foo.com";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_FALSE(does_match);
}

TEST(BatAdsUrlUtilTest,
    UrlMatchesEndWildcardPattern) {
  // Arrange
  const std::string url = "https://www.foo.com/bar?key=test";
  const std::string pattern = "https://www.foo.com/bar*";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_TRUE(does_match);
}

TEST(BatAdsUrlUtilTest,
    UrlMatchesMidWildcardPattern) {
  // Arrange
  const std::string url = "https://www.foo.com/woo-bar-hoo";
  const std::string pattern = "https://www.foo.com/woo*hoo";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_TRUE(does_match);
}

TEST(BatAdsUrlUtilTest,
    UrlDoesNotMatchMidWildcardPattern) {
  // Arrange
  const std::string url = "https://www.foo.com/woo";
  const std::string pattern = "https://www.foo.com/woo*hoo";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_FALSE(does_match);
}

TEST(BatAdsUrlUtilTest,
    SameSite) {
  // Arrange
  const std::string url1 = "https://foo.com?bar=test";
  const std::string url2 = "https://subdomain.foo.com/bar";

  // Act
  const bool is_same_site = SameSite(url1, url2);

  // Assert
  EXPECT_TRUE(is_same_site);
}

TEST(BatAdsUrlUtilTest,
    NotSameSite) {
  // Arrange
  const std::string url1 = "https://foo.com?bar=test";
  const std::string url2 = "https://subdomain.bar.com/foo";

  // Act
  const bool is_same_site = SameSite(url1, url2);

  // Assert
  EXPECT_FALSE(is_same_site);
}

TEST(BatAdsUrlUtilTest,
    SameSiteForUrlWithNoSubdomain) {
  // Arrange
  const std::string url1 = "https://foo.com?bar=test";
  const std::string url2 = "https://foo.com/bar";

  // Act
  const bool is_same_site = SameSite(url1, url2);

  // Assert
  EXPECT_TRUE(is_same_site);
}

TEST(BatAdsUrlUtilTest,
    NotSameSiteForUrlWithNoSubdomain) {
  // Arrange
  const std::string url1 = "https://foo.com?bar=test";
  const std::string url2 = "https://bar.com/foo";

  // Act
  const bool is_same_site = SameSite(url1, url2);

  // Assert
  EXPECT_FALSE(is_same_site);
}

TEST(BatAdsUrlUtilTest,
    SameSiteForUrlWithRef) {
  // Arrange
  const std::string url1 = "https://foo.com?bar=test#ref";
  const std::string url2 = "https://foo.com/bar";

  // Act
  const bool is_same_site = SameSite(url1, url2);

  // Assert
  EXPECT_TRUE(is_same_site);
}

TEST(BatAdsUrlUtilTest,
    NotSameSiteForUrlWithRef) {
  // Arrange
  const std::string url1 = "https://foo.com?bar=test#ref";
  const std::string url2 = "https://bar.com/foo";

  // Act
  const bool is_same_site = SameSite(url1, url2);

  // Assert
  EXPECT_FALSE(is_same_site);
}

}  // namespace ads
