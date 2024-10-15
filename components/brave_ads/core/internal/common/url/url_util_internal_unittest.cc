/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_util_internal.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::internal {

TEST(BraveAdsUrlUtilInternalTest, HasUrlSearchQueryNameAndValue) {
  // Act & Assert
  EXPECT_TRUE(HasSearchQuery(GURL("https://xyzzy.com/?search=thud")));
}

TEST(BraveAdsUrlUtilInternalTest, DoesNotHaveUrlSearchQueryName) {
  // Act & Assert
  EXPECT_FALSE(HasSearchQuery(GURL("https://xyzzy.com/?foo=bar")));
}

TEST(BraveAdsUrlUtilInternalTest, HasUrlSearchQueryNameAndEmptyValue) {
  // Act & Assert
  EXPECT_FALSE(HasSearchQuery(GURL("https://xyzzy.com/?search=")));
}

TEST(BraveAdsUrlUtilInternalTest, HasSearchQueryNameWithNoValue) {
  // Act & Assert
  EXPECT_FALSE(HasSearchQuery(GURL("https://xyzzy.com/?search")));
}

TEST(BraveAdsUrlUtilInternalTest, DoesETLDPlusOneContainWildcards) {
  // Act & Assert
  EXPECT_TRUE(DoesETLDPlusOneContainWildcards(GURL("https://*.appspot.com")));
  EXPECT_TRUE(DoesETLDPlusOneContainWildcards(GURL("https://*.com")));
}

TEST(BraveAdsUrlUtilInternalTest, DoesETLDPlusOneNotContainWildcards) {
  // Act & Assert
  EXPECT_FALSE(DoesETLDPlusOneContainWildcards(GURL("https://*.xyzzy.com")));
  EXPECT_FALSE(
      DoesETLDPlusOneContainWildcards(GURL("https://*.r.appspot.com")));
}

TEST(BraveAdsUrlUtilInternalTest, HostHasRegistryControlledDomain) {
  // Act & Assert
  EXPECT_TRUE(HostHasRegistryControlledDomain("mysite.appspot.com"));
  EXPECT_TRUE(HostHasRegistryControlledDomain("https://www.google.co.uk"));
  EXPECT_TRUE(HostHasRegistryControlledDomain("https://google.co.uk"));
  EXPECT_TRUE(HostHasRegistryControlledDomain("https://foo.aeroclub.aero"));
  EXPECT_TRUE(
      HostHasRegistryControlledDomain("https://example.mysite.appspot.com"));
}

TEST(BraveAdsUrlUtilInternalTest, HostDoesNotHaveRegistryControlledDomain) {
  // Act & Assert
  EXPECT_FALSE(HostHasRegistryControlledDomain("https://hello.unknown"));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithBraveSchemeAndFooBarHostName) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://foobar")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithBraveSchemeAndWalletHostName) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://wallet")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithBraveSchemeAndWalletHostNameAndPath) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://wallet/foo")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithBraveSchemeAndSyncHostName) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://sync")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithBraveSchemeAndSyncHostNameAndPath) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://sync/foo")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithBraveSchemeAndRewardsHostName) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://rewards")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithBraveSchemeAndRewardsHostNameAndPath) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://rewards/foo")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithBraveSchemeAndSettingsHostName) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://settings")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldNotSupportInternalUrlWithBraveSchemeAndSettingsHostNameAndFooBarPath) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://settings/foobar")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldSupportInternalUrlWithBraveSchemeAndSettingsHostNameAndSearchEnginesPath) {
  // Act & Assert
  EXPECT_TRUE(
      ShouldSupportInternalUrl(GURL("chrome://settings/searchEngines")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldSupportInternalUrlWithBraveSchemeAndSettingsHostNameSearchEnginesPathAndSearchQuery) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(
      GURL("chrome://settings/searchEngines?search=foobar")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldNotSupportInternalUrlWithBraveSchemeAndSettingsHostNameSearchEnginesPathAndMultipleSearchQueries) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(
      GURL("chrome://settings/searchEngines?search=foo&bar=baz")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldNotSupportInternalUrlWithBraveSchemeAndSettingsHostNameSearchEnginesPathAndInvalidQuery) {
  // Act & Assert
  EXPECT_FALSE(
      ShouldSupportInternalUrl(GURL("chrome://settings/searchEngines?search")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithBraveSchemeAndSettingsHostNameAndSearchPath) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://settings/search")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldSupportInternalUrlWithBraveSchemeAndSettingsHostNameSearchPathAndSearchQuery) {
  // Act & Assert
  EXPECT_TRUE(
      ShouldSupportInternalUrl(GURL("chrome://settings/search?search=foobar")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldNotSupportInternalUrlWithBraveSchemeAndSettingsHostNameSearchPathAndMultipleSearchQueries) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(
      GURL("chrome://settings/search?search=foo&bar=baz")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldNotSupportInternalUrlWithBraveSchemeAndSettingsHostNameSearchPathAndInvalidQuery) {
  // Act & Assert
  EXPECT_FALSE(
      ShouldSupportInternalUrl(GURL("chrome://settings/search?search")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithBraveSchemeAndSettingsHostNameAndQuery) {
  // Act & Assert
  EXPECT_FALSE(
      ShouldSupportInternalUrl(GURL("chrome://settings/?search=foobar")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldNotSupportInternalUrlWithBraveSchemeAndSettingsHostNameAndInvalidQuery) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://settings/?search")));
}

TEST(BraveAdsUrlUtilInternalTest, ShouldNotSupportMalformedUrl) {
  // Act & Assert
  EXPECT_FALSE(
      ShouldSupportInternalUrl(GURL("http://foobar.com/chrome://wallet")));
}

}  // namespace brave_ads::internal
