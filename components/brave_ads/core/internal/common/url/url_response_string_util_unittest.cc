/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsUrlResponseStringUtilTest,
     UrlResponseToStringIncludesHttpReasonPhrase) {
  // Arrange
  mojom::UrlResponseInfo mojom_url_response;
  mojom_url_response.url = GURL("https://brave.com");
  mojom_url_response.code = 404;
  mojom_url_response.body = "Not found";

  // Act & Assert
  EXPECT_EQ(
      "URL Response:\n"
      "  URL: https://brave.com/\n"
      "  Response Code: 404 Not Found\n"
      "  Response: Not found",
      UrlResponseToString(mojom_url_response));
}

TEST(BraveAdsUrlResponseStringUtilTest,
     UrlResponseToStringIncludesNetErrorNameForNegativeCode) {
  // Arrange
  mojom::UrlResponseInfo mojom_url_response;
  mojom_url_response.url = GURL("https://brave.com");
  mojom_url_response.code = -100;  // net::ERR_CONNECTION_CLOSED.
  mojom_url_response.body = "";

  // Act & Assert
  EXPECT_EQ(
      "URL Response:\n"
      "  URL: https://brave.com/\n"
      "  Response Code: -100 net::ERR_CONNECTION_CLOSED\n"
      "  Response: ",
      UrlResponseToString(mojom_url_response));
}

}  // namespace brave_ads
