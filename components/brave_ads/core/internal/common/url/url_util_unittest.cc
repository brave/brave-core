/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_util.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsUrlUtilTest, HttpsSchemeIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("https://foobar.com")));
}

TEST(BraveAdsUrlUtilTest, HttpSchemeIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("http://foobar.com")));
}

TEST(BraveAdsUrlUtilTest, FooBarSchemeIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("foobar://baz")));
}

TEST(BraveAdsUrlUtilTest, BraveSchemeWithFooBarHostNameIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("brave://foobar")));
}

TEST(BraveAdsUrlUtilTest, BraveSchemeWithWalletHostNameIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://wallet")));
}

TEST(BraveAdsUrlUtilTest, BraveSchemeWithWalletHostNameAndPathIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://wallet/foo")));
}

TEST(BraveAdsUrlUtilTest, BraveSchemeWithSyncHostNameIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://sync")));
}

TEST(BraveAdsUrlUtilTest, BraveSchemeWithSyncHostNameAndPathIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://sync/foo")));
}

TEST(BraveAdsUrlUtilTest, BraveSchemeWithRewardsHostNameIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://rewards")));
}

TEST(BraveAdsUrlUtilTest, BraveSchemeWithRewardsHostNameAndPathIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://rewards/foo")));
}

TEST(BraveAdsUrlUtilTest,
     BraveSchemeWithSettingsHostNameAndSearchEnginesPathIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://settings/searchEngines")));
}

TEST(BraveAdsUrlUtilTest,
     BraveSchemeWithSettingsHostNameAndSearchPathIsSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SchemeIsSupported(GURL("brave://settings/search")));
}

TEST(BraveAdsUrlUtilTest,
     BraveSchemeWithSettingsHostNameAndFooBarPathIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("brave://settings/foobar")));
}

TEST(BraveAdsUrlUtilTest, BraveSchemeWithSettingsHostNameIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("brave://settings")));
}

TEST(BraveAdsUrlUtilTest, MalformedUrlIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SchemeIsSupported(GURL("http://foobar.com/brave://wallet")));
}

TEST(BraveAdsUrlUtilTest, UrlMatchesPatternWithNoWildcards) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/"),
                              /*pattern*/ "https://www.foo.com/"));
}

TEST(BraveAdsUrlUtilTest, UrlWithPathMatchesPatternWithNoWildcards) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/bar"),
                              /*pattern*/ "https://www.foo.com/bar"));
}

TEST(BraveAdsUrlUtilTest, UrlDoesNotMatchPattern) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(
      MatchUrlPattern(GURL("https://www.foo.com/"), /*pattern*/ "www.foo.com"));
}

TEST(BraveAdsUrlUtilTest, UrlDoesNotMatchPatternWithMissingEmptyPath) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(MatchUrlPattern(GURL("https://www.foo.com/"),
                               /*pattern*/ "https://www.foo.com"));
}

TEST(BraveAdsUrlUtilTest, UrlMatchesEndWildcardPattern) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/bar?key=test"),
                              /*pattern*/ "https://www.foo.com/bar*"));
}

TEST(BraveAdsUrlUtilTest, UrlMatchesMidWildcardPattern) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/woo-bar-hoo"),
                              /*pattern*/ "https://www.foo.com/woo*hoo"));
}

TEST(BraveAdsUrlUtilTest, UrlDoesNotMatchMidWildcardPattern) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(MatchUrlPattern(GURL("https://www.foo.com/woo"),
                               /*pattern*/ "https://www.foo.com/woo*hoo"));
}

TEST(BraveAdsUrlUtilTest, SameDomainOrHost) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                               GURL("https://subdomain.foo.com/bar")));
}

TEST(BraveAdsUrlUtilTest, NotSameDomainOrHost) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                                GURL("https://subdomain.bar.com/foo")));
}

TEST(BraveAdsUrlUtilTest, SameDomainOrHostForUrlWithNoSubdomain) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                               GURL("https://foo.com/bar")));
}

TEST(BraveAdsUrlUtilTest, NotSameDomainOrHostForUrlWithNoSubdomain) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                                GURL("https://bar.com/foo")));
}

TEST(BraveAdsUrlUtilTest, SameDomainOrHostForUrlWithRef) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(SameDomainOrHost(GURL("https://foo.com?bar=test#ref"),
                               GURL("https://foo.com/bar")));
}

TEST(BraveAdsUrlUtilTest, NotSameDomainOrHostForUrlWithRef) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(SameDomainOrHost(GURL("https://foo.com?bar=test#ref"),
                                GURL("https://bar.com/foo")));
}

TEST(BraveAdsUrlUtilTest, DomainOrHostExists) {
  // Arrange
  const std::vector<GURL> urls = {GURL("https://foo.com"),
                                  GURL("https://bar.com")};

  // Act

  // Assert
  EXPECT_TRUE(DomainOrHostExists(urls, GURL("https://bar.com/foo")));
}

TEST(BraveAdsUrlUtilTest, DomainOrHostDoesNotExist) {
  // Arrange
  const std::vector<GURL> urls = {GURL("https://foo.com"),
                                  GURL("https://bar.com")};

  // Act

  // Assert
  EXPECT_FALSE(DomainOrHostExists(urls, GURL("https://baz.com/qux")));
}

}  // namespace brave_ads
