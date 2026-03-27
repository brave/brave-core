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
     ShouldNotSupportInternalUrlWithChromeSchemeAndFooBarHost) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://foobar")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithChromeSchemeAndGettingStartedHost) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://getting-started")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithChromeSchemeAndGettingStartedHostAndPath) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://getting-started/foo")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithChromeSchemeAndWalletHost) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://wallet")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithChromeSchemeAndWalletHostAndPath) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://wallet/foo")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithChromeSchemeAndWalletHostAndQuery) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://wallet?x=test")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithChromeSchemeAndSyncHost) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://sync")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithChromeSchemeAndSyncHostAndPath) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://sync/foo")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithChromeSchemeAndLeoAiHost) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://leo-ai")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithChromeSchemeAndLeoAiHostAndPath) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://leo-ai/foo")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithChromeSchemeAndRewardsHost) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://rewards")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithChromeSchemeAndRewardsHostAndPath) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://rewards/foo")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithChromeSchemeAndSettingsHost) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://settings")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithChromeSchemeAndSettingsHostAndFooBarPath) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://settings/foobar")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldSupportInternalUrlWithChromeSchemeAndSettingsHostAndSurveyPanelistPath) {
  // Act & Assert
  EXPECT_TRUE(
      ShouldSupportInternalUrl(GURL("chrome://settings/surveyPanelist")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldSupportInternalUrlWithChromeSchemeAndSettingsHostAndSearchEnginesPath) {
  // Act & Assert
  EXPECT_TRUE(
      ShouldSupportInternalUrl(GURL("chrome://settings/searchEngines")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldSupportInternalUrlWithChromeSchemeAndSettingsHostAndSearchEnginesDefaultSearchPath) {
  // Act & Assert
  EXPECT_TRUE(
      ShouldSupportInternalUrl(GURL("chrome://settings/search/defaultSearch")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldSupportInternalUrlWithChromeSchemeAndSettingsHostSearchEnginesPathAndSearchQuery) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(
      GURL("chrome://settings/searchEngines?search=foobar")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldNotSupportInternalUrlWithChromeSchemeAndSettingsHostSearchEnginesPathAndMultipleSearchQueries) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(
      GURL("chrome://settings/searchEngines?search=foo&bar=baz")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldNotSupportInternalUrlWithChromeSchemeAndSettingsHostSearchEnginesPathAndInvalidQuery) {
  // Act & Assert
  EXPECT_FALSE(
      ShouldSupportInternalUrl(GURL("chrome://settings/searchEngines?search")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldSupportInternalUrlWithChromeSchemeAndSettingsHostAndSearchPath) {
  // Act & Assert
  EXPECT_TRUE(ShouldSupportInternalUrl(GURL("chrome://settings/search")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldSupportInternalUrlWithChromeSchemeAndSettingsHostSearchPathAndSearchQuery) {
  // Act & Assert
  EXPECT_TRUE(
      ShouldSupportInternalUrl(GURL("chrome://settings/search?search=foobar")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldNotSupportInternalUrlWithChromeSchemeAndSettingsHostSearchPathAndMultipleSearchQueries) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(
      GURL("chrome://settings/search?search=foo&bar=baz")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldNotSupportInternalUrlWithChromeSchemeAndSettingsHostSearchPathAndInvalidQuery) {
  // Act & Assert
  EXPECT_FALSE(
      ShouldSupportInternalUrl(GURL("chrome://settings/search?search")));
}

TEST(BraveAdsUrlUtilInternalTest,
     ShouldNotSupportInternalUrlWithChromeSchemeAndSettingsHostAndQuery) {
  // Act & Assert
  EXPECT_FALSE(
      ShouldSupportInternalUrl(GURL("chrome://settings/?search=foobar")));
}

TEST(
    BraveAdsUrlUtilInternalTest,
    ShouldNotSupportInternalUrlWithChromeSchemeAndSettingsHostAndInvalidQuery) {
  // Act & Assert
  EXPECT_FALSE(ShouldSupportInternalUrl(GURL("chrome://settings/?search")));
}

TEST(BraveAdsUrlUtilInternalTest, ShouldNotSupportMalformedUrl) {
  // Act & Assert
  EXPECT_FALSE(
      ShouldSupportInternalUrl(GURL("http://foobar.com/chrome://wallet")));
}

}  // namespace brave_ads::internal
