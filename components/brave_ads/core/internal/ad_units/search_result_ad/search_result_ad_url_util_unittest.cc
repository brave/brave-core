/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_url_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSearchResultAdUrlUtilTest,
     IsSearchResultAdRedirectUrlForProductionHost) {
  // Act & Assert
  EXPECT_TRUE(IsSearchResultAdRedirectUrl(
      GURL("https://search.brave.com/a/redirect?placement_id=1")));
}

TEST(BraveAdsSearchResultAdUrlUtilTest,
     IsSearchResultAdRedirectUrlForStagingHost) {
  // Act & Assert
  EXPECT_TRUE(IsSearchResultAdRedirectUrl(
      GURL("https://search.bravesoftware.com/a/redirect?placement_id=1")));
}

TEST(BraveAdsSearchResultAdUrlUtilTest,
     IsSearchResultAdRedirectUrlWithoutQuery) {
  // Act & Assert
  EXPECT_TRUE(
      IsSearchResultAdRedirectUrl(GURL("https://search.brave.com/a/redirect")));
}

TEST(BraveAdsSearchResultAdUrlUtilTest,
     IsNotSearchResultAdClickRedirectUrlForInvalidUrl) {
  // Act & Assert
  EXPECT_FALSE(IsSearchResultAdRedirectUrl(GURL("")));
  EXPECT_FALSE(IsSearchResultAdRedirectUrl(GURL("not a url")));
}

TEST(BraveAdsSearchResultAdUrlUtilTest,
     IsNotSearchResultAdClickRedirectUrlForNonHttpsScheme) {
  // Act & Assert
  EXPECT_FALSE(IsSearchResultAdRedirectUrl(
      GURL("http://search.brave.com/a/redirect?placement_id=1")));
  EXPECT_FALSE(IsSearchResultAdRedirectUrl(
      GURL("http://search.bravesoftware.com/a/redirect?placement_id=1")));
}

TEST(BraveAdsSearchResultAdUrlUtilTest,
     IsNotSearchResultAdClickRedirectUrlForWrongPath) {
  // Act & Assert
  EXPECT_FALSE(IsSearchResultAdRedirectUrl(
      GURL("https://search.brave.com/search?q=foo")));
  EXPECT_FALSE(IsSearchResultAdRedirectUrl(
      GURL("https://search.bravesoftware.com/search?q=foo")));
}

TEST(BraveAdsSearchResultAdUrlUtilTest,
     IsNotSearchResultAdClickRedirectUrlForNonBraveSearchHost) {
  // Act & Assert
  EXPECT_FALSE(IsSearchResultAdRedirectUrl(
      GURL("https://example.com/a/redirect?placement_id=1")));
}

}  // namespace brave_ads
