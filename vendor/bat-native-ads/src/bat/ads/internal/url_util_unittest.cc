/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/url_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace ads {

class BraveAdsUrlUtilTest : public ::testing::Test {
 protected:
  BraveAdsUrlUtilTest() {
    // You can do set-up work for each test here
  }

  ~BraveAdsUrlUtilTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
};

TEST_F(BraveAdsUrlUtilTest,
    GetUrlForUrlMissingScheme) {
  // Arrange
  const std::string url = "www.foobar.com";

  // Act
  const std::string url_with_schema = GetUrlWithScheme(url);

  // Assert
  const std::string expected_url_with_scheme = "https://www.foobar.com";
  EXPECT_EQ(expected_url_with_scheme, url_with_schema);
}

TEST_F(BraveAdsUrlUtilTest,
    GetUrlForUrlWithHttpScheme) {
  // Arrange
  const std::string url = "http://www.foobar.com";

  // Act
  const std::string url_with_schema = GetUrlWithScheme(url);

  // Assert
  const std::string expected_url_with_scheme = "http://www.foobar.com";
  EXPECT_EQ(expected_url_with_scheme, url_with_schema);
}

TEST_F(BraveAdsUrlUtilTest,
    GetUrlForUrlWithHttpsScheme) {
  // Arrange
  const std::string url = "https://www.foobar.com";

  // Act
  const std::string url_with_schema = GetUrlWithScheme(url);

  // Assert
  const std::string expected_url_with_scheme = "https://www.foobar.com";
  EXPECT_EQ(expected_url_with_scheme, url_with_schema);
}

TEST_F(BraveAdsUrlUtilTest,
    UrlMatchesPatternWithNoWildcards) {
  // Arrange
  const std::string url = "https://www.foo.com/";
  const std::string pattern = "https://www.foo.com/";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_TRUE(does_match);
}

TEST_F(BraveAdsUrlUtilTest,
    UrlDoesNotMatchPattern) {
  // Arrange
  const std::string url = "https://www.foo.com/";
  const std::string pattern = "www.foo.com";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_FALSE(does_match);
}

TEST_F(BraveAdsUrlUtilTest,
    UrlDoesNotMatchPatternWithMissingEmptyPath) {
  // Arrange
  const std::string url = "https://www.foo.com/";
  const std::string pattern = "https://www.foo.com";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_FALSE(does_match);
}

TEST_F(BraveAdsUrlUtilTest,
    UrlMatchesEndWildcardPattern) {
  // Arrange
  const std::string url = "https://www.foo.com/bar?key=test";
  const std::string pattern = "https://www.foo.com/bar*";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_TRUE(does_match);
}

TEST_F(BraveAdsUrlUtilTest,
    UrlMatchesMidWildcardPattern) {
  // Arrange
  const std::string url = "https://www.foo.com/woo-bar-hoo";
  const std::string pattern = "https://www.foo.com/woo*hoo";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_TRUE(does_match);
}

TEST_F(BraveAdsUrlUtilTest,
    UrlDoesNotMatchMidWildcardPattern) {
  // Arrange
  const std::string url = "https://www.foo.com/woo";
  const std::string pattern = "https://www.foo.com/woo*hoo";

  // Act
  const bool does_match = UrlMatchesPattern(url, pattern);

  // Assert
  EXPECT_FALSE(does_match);
}

TEST_F(BraveAdsUrlUtilTest,
    SameSite) {
  // Arrange
  const std::string url1 = "https://foo.com?bar=test";
  const std::string url2 = "https://subdomain.foo.com/bar";

  // Act
  const bool is_same_site = SameSite(url1, url2);

  // Assert
  EXPECT_TRUE(is_same_site);
}

TEST_F(BraveAdsUrlUtilTest,
    NotSameSite) {
  // Arrange
  const std::string url1 = "https://foo.com?bar=test";
  const std::string url2 = "https://subdomain.bar.com/foo";

  // Act
  const bool is_same_site = SameSite(url1, url2);

  // Assert
  EXPECT_FALSE(is_same_site);
}

TEST_F(BraveAdsUrlUtilTest,
    SameSiteForUrlWithNoSubdomain) {
  // Arrange
  const std::string url1 = "https://foo.com?bar=test";
  const std::string url2 = "https://foo.com/bar";

  // Act
  const bool is_same_site = SameSite(url1, url2);

  // Assert
  EXPECT_TRUE(is_same_site);
}

TEST_F(BraveAdsUrlUtilTest,
    NotSameSiteForUrlWithNoSubdomain) {
  // Arrange
  const std::string url1 = "https://foo.com?bar=test";
  const std::string url2 = "https://bar.com/foo";

  // Act
  const bool is_same_site = SameSite(url1, url2);

  // Assert
  EXPECT_FALSE(is_same_site);
}

}  // namespace ads
