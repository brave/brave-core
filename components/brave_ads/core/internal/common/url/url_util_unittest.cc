/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsUrlUtilTest, GetUrlExcludingQuery) {
  // Act & Assert
  EXPECT_EQ(GURL("https://foo.com/bar"),
            GetUrlExcludingQuery(GURL("https://foo.com/bar?baz=qux")));
}

TEST(BraveAdsUrlUtilTest, GetUrlExcludingQueryWhenNoQueryNameAndValue) {
  // Act & Assert
  EXPECT_EQ(GURL("https://foo.com/bar"),
            GetUrlExcludingQuery(GURL("https://foo.com/bar?")));
}

TEST(BraveAdsUrlUtilTest, GetUrlExcludingQueryWhenNoQuery) {
  // Act & Assert
  EXPECT_EQ(GURL("https://foo.com/bar"),
            GetUrlExcludingQuery(GURL("https://foo.com/bar")));
}

TEST(BraveAdsUrlUtilTest, ShouldNotSupportInvalidUrl) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("some random string")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("//*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("://*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("*://*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("*****")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://?.com")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://?.google.com")));
}

TEST(BraveAdsUrlUtilTest, ShouldNotSupportUrlWithNonHttpsScheme) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("*://www.example.com/*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("http://www.example.com/*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("ftp://www.example.com/*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL(
      "ipfs://bafybeigi77rim3p5tw3upw2ca4ep5ng7uaarvrz46zidd2ai6cjh46yxoy/")));
  EXPECT_FALSE(ShouldSupportUrl(GURL(
      "ipfs://bafybeigi77rim3p5tw3upw2ca4ep5ng7uaarvrz46zidd2ai6cjh46yxoy/")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("javascript:alert(1)")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("data:text/html,<h1>hello</h1>")));
}

TEST(BraveAdsUrlUtilTest, ShouldNotSupportUrlWithPortNumber) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://www.brave.com:1234/thank-you*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://www.brave.com:1234*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://brave.com:*/x")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://brave.com:*")));
}

TEST(BraveAdsUrlUtilTest, ShouldNotSupportIpAddress) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://1.2.3.4/thank-you*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://192.168.0.0/*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://192.*.*.*/x")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://[::1]/thankyou")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://030000001017/")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://0xc000020f/")));
}

TEST(BraveAdsUrlUtilTest, ShouldNotSupportUrlWithUsernameOrPassword) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://user:pwd@brave.com/thank-you*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://user@brave.com:1234*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://*@brave.com/x")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://user:*@brave.com/y")));
}

TEST(BraveAdsUrlUtilTest, ShouldNotSupportUrlWithWildcardInEtldPlus1) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://www.google.co.*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://www.google.*.uk")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://www.*.co.uk")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://www.google.co.uk*")));
  EXPECT_FALSE(
      ShouldSupportUrl(GURL("https://www.comparecredit.com*/secure*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://www.sophos.com*thank-you*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://*.security/*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://*.appspot.com/*")));
}

TEST(BraveAdsUrlUtilTest, ShouldNotSupportLocalUrl) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://localhost/*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://example.local/*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://*.example.local/*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://*.local/*")));
  EXPECT_FALSE(ShouldSupportUrl(GURL("https://localhost*/")));
}

TEST(BraveAdsUrlUtilTest, ShouldSupportUrl) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportUrl(GURL("https://*.google.co.uk")));
  EXPECT_TRUE(ShouldSupportUrl(GURL("https://www.google.co.uk/*")));
  EXPECT_TRUE(
      ShouldSupportUrl(GURL("https://dashboard.0x.org/create-account/"
                            "verification-sent?tx-relay-brave*")));
  EXPECT_TRUE(
      ShouldSupportUrl(GURL("https://www.app.apxlending.com/verify_email*")));
  EXPECT_TRUE(ShouldSupportUrl(GURL("https://bonkmark.com/*/#send")));
  EXPECT_TRUE(ShouldSupportUrl(GURL("https://account.brave.com/account/*")));
  EXPECT_TRUE(ShouldSupportUrl(GURL("https://www.cube.exchange/*step=5*")));
  EXPECT_TRUE(ShouldSupportUrl(GURL(
      "https://play.google.com/store/apps/details?id=com.musclewiki.macro*")));
  EXPECT_TRUE(ShouldSupportUrl(
      GURL("https://mail.proton.me/u/*/inbox?welcome=true&ref=*")));
  EXPECT_TRUE(ShouldSupportUrl(GURL("https://sheets.new/*")));
  EXPECT_TRUE(ShouldSupportUrl(GURL("https://*.hello.security/*")));
  EXPECT_TRUE(ShouldSupportUrl(GURL("https://mysite.appspot.com/*")));
  EXPECT_TRUE(ShouldSupportUrl(GURL("https://my.site.developer.app/*")));
}

TEST(BraveAdsUrlUtilTest, ShouldNotSupportBraveSchemeWithFooBarHostName) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("chrome://foobar")));
}

TEST(BraveAdsUrlUtilTest, ShouldSupportBraveSchemeWithWalletHostName) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportUrl(GURL("chrome://wallet")));
}

TEST(BraveAdsUrlUtilTest, ShouldSupportBraveSchemeWithWalletHostNameAndPath) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportUrl(GURL("chrome://wallet/foo")));
}

TEST(BraveAdsUrlUtilTest, ShouldSupportBraveSchemeWithSyncHostName) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportUrl(GURL("chrome://sync")));
}

TEST(BraveAdsUrlUtilTest, ShouldSupportBraveSchemeWithSyncHostNameAndPath) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportUrl(GURL("chrome://sync/foo")));
}

TEST(BraveAdsUrlUtilTest, ShouldSupportBraveSchemeWithRewardsHostName) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportUrl(GURL("chrome://rewards")));
}

TEST(BraveAdsUrlUtilTest, ShouldSupportBraveSchemeWithRewardsHostNameAndPath) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportUrl(GURL("chrome://rewards/foo")));
}

TEST(BraveAdsUrlUtilTest, ShouldNotSupportBraveSchemeWithSettingsHostName) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("chrome://settings")));
}

TEST(BraveAdsUrlUtilTest,
     ShouldNotSupportBraveSchemeWithSettingsHostNameAndFooBarPath) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("chrome://settings/foobar")));
}

TEST(BraveAdsUrlUtilTest,
     ShouldSupportBraveSchemeWithSettingsHostNameAndSearchEnginesPath) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportUrl(GURL("chrome://settings/searchEngines")));
}

TEST(
    BraveAdsUrlUtilTest,
    ShouldSupportBraveSchemeWithSettingsHostNameSearchEnginesPathAndSearchQuery) {
  // Act & Assert
  EXPECT_TRUE(
      ShouldSupportUrl(GURL("chrome://settings/searchEngines?search=foobar")));
}

TEST(
    BraveAdsUrlUtilTest,
    ShouldNotSupportBraveSchemeWithSettingsHostNameSearchEnginesPathAndMultipleSearchQueries) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(
      GURL("chrome://settings/searchEngines?search=foo&bar=baz")));
}

TEST(
    BraveAdsUrlUtilTest,
    ShouldNotSupportBraveSchemeWithSettingsHostNameSearchEnginesPathAndInvalidQuery) {
  // Act & Assert
  EXPECT_FALSE(
      ShouldSupportUrl(GURL("chrome://settings/searchEngines?search")));
}

TEST(BraveAdsUrlUtilTest,
     ShouldSupportBraveSchemeWithSettingsHostNameAndSearchPath) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportUrl(GURL("chrome://settings/search")));
}

TEST(BraveAdsUrlUtilTest,
     ShouldSupportBraveSchemeWithSettingsHostNameSearchPathAndSearchQuery) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportUrl(GURL("chrome://settings/search?search=foobar")));
}

TEST(
    BraveAdsUrlUtilTest,
    ShouldNotSupportBraveSchemeWithSettingsHostNameSearchPathAndMultipleSearchQueries) {
  // Act & Assert
  EXPECT_FALSE(
      ShouldSupportUrl(GURL("chrome://settings/search?search=foo&bar=baz")));
}

TEST(BraveAdsUrlUtilTest,
     ShouldNotSupportBraveSchemeWithSettingsHostNameSearchPathAndInvalidQuery) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("chrome://settings/search?search")));
}

TEST(BraveAdsUrlUtilTest,
     ShouldNotSupportBraveSchemeWithSettingsHostNameAndQuery) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("chrome://settings/?search=foobar")));
}

TEST(BraveAdsUrlUtilTest,
     ShouldNotSupportBraveSchemeWithSettingsHostNameAndInvalidQuery) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("chrome://settings/?search")));
}

TEST(BraveAdsUrlUtilTest, ShouldNotSupportMalformedUrl) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportUrl(GURL("http://foobar.com/chrome://wallet")));
}

TEST(BraveAdsUrlUtilTest, UrlMatchesPatternWithNoWildcards) {
  // Act & Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/"),
                              /*pattern=*/"https://www.foo.com/"));
}

TEST(BraveAdsUrlUtilTest, UrlWithPathMatchesPatternWithNoWildcards) {
  // Act & Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/bar"),
                              /*pattern=*/"https://www.foo.com/bar"));
}

TEST(BraveAdsUrlUtilTest, UrlDoesNotMatchPattern) {
  // Act & Assert
  EXPECT_FALSE(
      MatchUrlPattern(GURL("https://www.foo.com/"), /*pattern=*/"www.foo.com"));
}

TEST(BraveAdsUrlUtilTest, UrlMatchesPatternWithQuery) {
  // Act & Assert
  EXPECT_TRUE(
      MatchUrlPattern(GURL("https://dashboard.0x.org/create-account/"
                           "verification-sent?tx-relay-brave-browser"),
                      /*pattern=*/"https://dashboard.0x.org/create-account/"
                                  "verification-sent?tx-relay-brave*"));
}

TEST(BraveAdsUrlUtilTest, UrlDoesNotMatchPatternWithQuery) {
  // Act & Assert
  EXPECT_FALSE(
      MatchUrlPattern(GURL("https://dashboard.0x.org/create-account/"
                           "verification-sent-tx-relay-brave-browser"),
                      /*pattern=*/"https://dashboard.0x.org/create-account/"
                                  "verification-sent?tx-relay-brave*"));
}

TEST(BraveAdsUrlUtilTest, UrlDoesNotMatchPatternWithMissingEmptyPath) {
  // Act & Assert
  EXPECT_FALSE(MatchUrlPattern(GURL("https://www.foo.com/"),
                               /*pattern=*/"https://www.foo.com"));
}

TEST(BraveAdsUrlUtilTest, UrlMatchesEndWildcardPattern) {
  // Act & Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/bar?key=test"),
                              /*pattern=*/"https://www.foo.com/bar*"));
}

TEST(BraveAdsUrlUtilTest, UrlMatchesMidWildcardPattern) {
  // Act & Assert
  EXPECT_TRUE(MatchUrlPattern(GURL("https://www.foo.com/woo-bar-hoo"),
                              /*pattern=*/"https://www.foo.com/woo*hoo"));
}

TEST(BraveAdsUrlUtilTest, UrlDoesNotMatchMidWildcardPattern) {
  // Act & Assert
  EXPECT_FALSE(MatchUrlPattern(GURL("https://www.foo.com/woo"),
                               /*pattern=*/"https://www.foo.com/woo*hoo"));
}

TEST(BraveAdsUrlUtilTest, SameDomainOrHost) {
  // Act & Assert
  EXPECT_TRUE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                               GURL("https://subdomain.foo.com/bar")));
}

TEST(BraveAdsUrlUtilTest, NotSameDomainOrHost) {
  // Act & Assert
  EXPECT_FALSE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                                GURL("https://subdomain.bar.com/foo")));
}

TEST(BraveAdsUrlUtilTest, SameDomainOrHostForUrlWithNoSubdomain) {
  // Act & Assert
  EXPECT_TRUE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                               GURL("https://foo.com/bar")));
}

TEST(BraveAdsUrlUtilTest, NotSameDomainOrHostForUrlWithNoSubdomain) {
  // Act & Assert
  EXPECT_FALSE(SameDomainOrHost(GURL("https://foo.com?bar=test"),
                                GURL("https://bar.com/foo")));
}

TEST(BraveAdsUrlUtilTest, SameDomainOrHostForUrlWithRef) {
  // Act & Assert
  EXPECT_TRUE(SameDomainOrHost(GURL("https://foo.com?bar=test#ref"),
                               GURL("https://foo.com/bar")));
}

TEST(BraveAdsUrlUtilTest, NotSameDomainOrHostForUrlWithRef) {
  // Act & Assert
  EXPECT_FALSE(SameDomainOrHost(GURL("https://foo.com?bar=test#ref"),
                                GURL("https://bar.com/foo")));
}

TEST(BraveAdsUrlUtilTest, DomainOrHostExists) {
  // Arrange
  const std::vector<GURL> urls = {GURL("https://foo.com"),
                                  GURL("https://bar.com")};

  // Act & Assert
  EXPECT_TRUE(DomainOrHostExists(urls, GURL("https://bar.com/foo")));
}

TEST(BraveAdsUrlUtilTest, DomainOrHostDoesNotExist) {
  // Arrange
  const std::vector<GURL> urls = {GURL("https://foo.com"),
                                  GURL("https://bar.com")};

  // Act & Assert
  EXPECT_FALSE(DomainOrHostExists(urls, GURL("https://baz.com/qux")));
}

}  // namespace brave_ads
