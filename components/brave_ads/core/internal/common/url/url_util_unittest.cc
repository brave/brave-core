/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsUrlUtilTest, GetUrlWithEmptyQuery) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(GURL("https://foo.com/bar"),
            GetUrlWithEmptyQuery(GURL("https://foo.com/bar?baz=qux")));
}

TEST(BraveAdsUrlUtilTest, DoesNotSupportInvalidUrl) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportUrl(GURL("INVALID")));
}

TEST(BraveAdsUrlUtilTest, DoesSupportUrlWithHttpsScheme) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesSupportUrl(GURL("https://foobar.com")));
}

TEST(BraveAdsUrlUtilTest, DoesNotSupportUrlWithHttpScheme) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportUrl(GURL("http://foobar.com")));
}

TEST(BraveAdsUrlUtilTest, DoesNotSupportUrlWithFooBarScheme) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportUrl(GURL("foobar://baz")));
}

TEST(BraveAdsUrlUtilTest, DoesNotSupportBraveSchemeWithFooBarHostName) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportUrl(GURL("brave://foobar")));
}

TEST(BraveAdsUrlUtilTest, DoesSupportBraveSchemeWithWalletHostName) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesSupportUrl(GURL("brave://wallet")));
}

TEST(BraveAdsUrlUtilTest, DoesSupportBraveSchemeWithWalletHostNameAndPath) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesSupportUrl(GURL("brave://wallet/foo")));
}

TEST(BraveAdsUrlUtilTest, DoesSupportBraveSchemeWithSyncHostName) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesSupportUrl(GURL("brave://sync")));
}

TEST(BraveAdsUrlUtilTest, DoesSupportBraveSchemeWithSyncHostNameAndPath) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesSupportUrl(GURL("brave://sync/foo")));
}

TEST(BraveAdsUrlUtilTest, DoesSupportBraveSchemeWithRewardsHostName) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesSupportUrl(GURL("brave://rewards")));
}

TEST(BraveAdsUrlUtilTest, DoesSupportBraveSchemeWithRewardsHostNameAndPath) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesSupportUrl(GURL("brave://rewards/foo")));
}

TEST(BraveAdsUrlUtilTest, DoesNotSupportBraveSchemeWithSettingsHostName) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportUrl(GURL("brave://settings")));
}

TEST(BraveAdsUrlUtilTest,
     DoesNotSupportBraveSchemeWithSettingsHostNameAndFooBarPath) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportUrl(GURL("brave://settings/foobar")));
}

TEST(BraveAdsUrlUtilTest,
     DoesSupportBraveSchemeWithSettingsHostNameAndSearchEnginesPath) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesSupportUrl(GURL("brave://settings/searchEngines")));
}

TEST(
    BraveAdsUrlUtilTest,
    DoesSupportBraveSchemeWithSettingsHostNameSearchEnginesPathAndSearchQuery) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(
      DoesSupportUrl(GURL("brave://settings/searchEngines?search=foobar")));
}

TEST(
    BraveAdsUrlUtilTest,
    DoesNotSupportBraveSchemeWithSettingsHostNameSearchEnginesPathAndMultipleSearchQueries) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportUrl(
      GURL("brave://settings/searchEngines?search=foo&bar=baz")));
}

TEST(
    BraveAdsUrlUtilTest,
    DoesNotSupportBraveSchemeWithSettingsHostNameSearchEnginesPathAndInvalidQuery) {
  // Arrange

  // Act
  // Assert
  EXPECT_FALSE(DoesSupportUrl(GURL("brave://settings/searchEngines?search")));
}

TEST(BraveAdsUrlUtilTest,
     DoesSupportBraveSchemeWithSettingsHostNameAndSearchPath) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesSupportUrl(GURL("brave://settings/search")));
}

TEST(BraveAdsUrlUtilTest,
     DoesSupportBraveSchemeWithSettingsHostNameSearchPathAndSearchQuery) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(DoesSupportUrl(GURL("brave://settings/search?search=foobar")));
}

TEST(
    BraveAdsUrlUtilTest,
    DoesNotSupportBraveSchemeWithSettingsHostNameSearchPathAndMultipleSearchQueries) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(
      DoesSupportUrl(GURL("brave://settings/search?search=foo&bar=baz")));
}

TEST(BraveAdsUrlUtilTest,
     DoesNotSupportBraveSchemeWithSettingsHostNameSearchPathAndInvalidQuery) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportUrl(GURL("brave://settings/search?search")));
}

TEST(BraveAdsUrlUtilTest,
     DoesNotSupportBraveSchemeWithSettingsHostNameAndQuery) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportUrl(GURL("brave://settings/?search=foobar")));
}

TEST(BraveAdsUrlUtilTest,
     DoesNotSupportBraveSchemeWithSettingsHostNameAndInvalidQuery) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportUrl(GURL("brave://settings/?search")));
}

TEST(BraveAdsUrlUtilTest, MalformedUrlIsNotSupported) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(DoesSupportUrl(GURL("http://foobar.com/brave://wallet")));
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
