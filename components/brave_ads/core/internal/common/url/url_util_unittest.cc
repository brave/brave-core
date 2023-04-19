/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_util.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

TEST(BatAdsUrlUtilTest, HttpsSchemeIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("https://foobar.com")));
}

TEST(BatAdsUrlUtilTest, HttpSchemeIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("http://foobar.com")));
}

TEST(BatAdsUrlUtilTest, FooBarSchemeIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("foobar://baz")));
}

TEST(BatAdsUrlUtilTest, BraveSchemeWithFooBarHostNameIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("brave://foobar")));
}

TEST(BatAdsUrlUtilTest, BraveSchemeWithWalletHostNameIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://wallet")));
}

TEST(BatAdsUrlUtilTest, BraveSchemeWithWalletHostNameAndPathIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://wallet/foo")));
}

TEST(BatAdsUrlUtilTest, BraveSchemeWithSyncHostNameIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://sync")));
}

TEST(BatAdsUrlUtilTest, BraveSchemeWithSyncHostNameAndPathIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://sync/foo")));
}

TEST(BatAdsUrlUtilTest, BraveSchemeWithRewardsHostNameIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://rewards")));
}

TEST(BatAdsUrlUtilTest, BraveSchemeWithRewardsHostNameAndPathIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://rewards/foo")));
}

TEST(BatAdsUrlUtilTest,
     BraveSchemeWithSettingsHostNameAndSearchEnginesPathIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://settings/searchEngines")));
}

TEST(BatAdsUrlUtilTest,
     BraveSchemeWithSettingsHostNameAndSearchPathIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://settings/search")));
}

TEST(BatAdsUrlUtilTest,
     BraveSchemeWithSettingsHostNameAndFooBarPathIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("brave://settings/foobar")));
}

TEST(BatAdsUrlUtilTest, BraveSchemeWithSettingsHostNameIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("brave://settings")));
}

TEST(BatAdsUrlUtilTest, MalformedUrlIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("http://foobar.com/brave://wallet")));
}

TEST(BatAdsUrlUtilTest, UrlMatchesPatternWithNoWildcards) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/"),
                              /*pattern*/ "https://www.foo.com/"));
}

TEST(BatAdsUrlUtilTest, UrlWithPathMatchesPatternWithNoWildcards) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/bar"),
                              /*pattern*/ "https://www.foo.com/bar"));
}

TEST(BatAdsUrlUtilTest, UrlDoesNotMatchPattern) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(
      MatchUrlPattern(GURL("https://www.foo.com/"), /*pattern*/ "www.foo.com"));
}

TEST(BatAdsUrlUtilTest, UrlDoesNotMatchPatternWithMissingEmptyPath) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(MatchUrlPattern(GURL("https://www.foo.com/"),
                               /*pattern*/ "https://www.foo.com"));
}

TEST(BatAdsUrlUtilTest, UrlMatchesEndWildcardPattern) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/bar?key=test"),
                              /*pattern*/ "https://www.foo.com/bar*"));
}

TEST(BatAdsUrlUtilTest, UrlMatchesMidWildcardPattern) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/woo-bar-hoo"),
                              /*pattern*/ "https://www.foo.com/woo*hoo"));
}

TEST(BatAdsUrlUtilTest, UrlDoesNotMatchMidWildcardPattern) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(MatchUrlPattern(GURL("https://www.foo.com/woo"),
                               /*pattern*/ "https://www.foo.com/woo*hoo"));
}

TEST(BatAdsUrlUtilTest, SameDomainOrHost) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                               GURL("https://subdomain.foo.com/bar")));
}

TEST(BatAdsUrlUtilTest, NotSameDomainOrHost) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                                GURL("https://subdomain.bar.com/foo")));
}

TEST(BatAdsUrlUtilTest, SameDomainOrHostForUrlWithNoSubdomain) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                               GURL("https://foo.com/bar")));
}

TEST(BatAdsUrlUtilTest, NotSameDomainOrHostForUrlWithNoSubdomain) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                                GURL("https://bar.com/foo")));
}

TEST(BatAdsUrlUtilTest, SameDomainOrHostForUrlWithRef) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SameDomainOrHost(GURL("https://foo.com?bar=test#ref"),
                               GURL("https://foo.com/bar")));
}

TEST(BatAdsUrlUtilTest, NotSameDomainOrHostForUrlWithRef) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SameDomainOrHost(GURL("https://foo.com?bar=test#ref"),
                                GURL("https://bar.com/foo")));
}

TEST(BatAdsUrlUtilTest, DomainOrHostExists) {
  // Arrange
  const std::vector<GURL> urls = {GURL("https://foo.com"),
                                  GURL("https://bar.com")};

  // Act

  // Assert
  EXPECT_TRUE(DomainOrHostExists(urls, GURL("https://bar.com/foo")));
}

TEST(BatAdsUrlUtilTest, DomainOrHostDoesNotExist) {
  // Arrange
  const std::vector<GURL> urls = {GURL("https://foo.com"),
                                  GURL("https://bar.com")};

  // Act

  // Assert
  EXPECT_FALSE(DomainOrHostExists(urls, GURL("https://baz.com/qux")));
}

}  // namespace brave_ads
