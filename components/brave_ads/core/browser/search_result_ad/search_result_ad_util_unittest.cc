/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/search_result_ad/search_result_ad_util.h"
#include "base/strings/string_piece.h"
#include "services/network/public/cpp/resource_request.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep
#include "url/gurl.h"

namespace brave_ads {

TEST(SearchResultAdUtilTest, CheckSearchResultAdClickedConfirmationUrl) {
  EXPECT_TRUE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v3/click")));
  EXPECT_TRUE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v3/click")));

  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("http://search.anonymous.ads.brave.com/v3/click")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("http://search.anonymous.ads.bravesoftware.com/v3/click")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v4/click")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v4/click")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v3/non")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v3/non")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.brave.com/v3/confirmation")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.bravesoftware.com/v3/click")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com/v3")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com/v3")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.brave.com")));
  EXPECT_FALSE(IsSearchResultAdClickedConfirmationUrl(
      GURL("https://search.anonymous.ads.bravesoftware.com")));
}

TEST(SearchResultAdUtilTest, CheckGetClickedSearchResultAdCreativeInstanceId) {
  GURL request_url = GURL(
      "https://search.anonymous.ads.brave.com/v3/click?"
      "creativeInstanceId=creative-instance-id");
  EXPECT_EQ("creative-instance-id",
            GetClickedSearchResultAdCreativeInstanceId(request_url));

  request_url = GURL(
      "https://search.anonymous.ads.brave.com/v3/click?"
      "creativeInstanceId=");
  EXPECT_TRUE(GetClickedSearchResultAdCreativeInstanceId(request_url).empty());

  request_url = GURL(
      "https://search.anonymous.ads.brave.com/v3/click?"
      "creativeInstance=creative-instance-id");
  EXPECT_TRUE(GetClickedSearchResultAdCreativeInstanceId(request_url).empty());

  request_url = GURL("https://search.anonymous.ads.brave.com/v3/click");
  EXPECT_TRUE(GetClickedSearchResultAdCreativeInstanceId(request_url).empty());
}

}  // namespace brave_ads
