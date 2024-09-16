/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code_util.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsHttpStatusCodeUtilTest, HttpStatusCodeToString) {
  // Act & Assert
  for (int i = 0; i <= 1024; ++i) {
    const std::optional<std::string> http_status_code =
        HttpStatusCodeToString(i);
    if (!http_status_code) {
      // Nonsensical HTTP status code.
      continue;
    }

    // Allowed HTTP status codes, other codes are mapped to their class.
    if (i == 400 ||  // Bad Request.
        i == 401 ||  // Unauthorized.
        i == 403 ||  // Forbidden.
        i == 404 ||  // Not Found.
        i == 407 ||  // Proxy Authentication Required.
        i == 408 ||  // Request Timeout.
        i == 429 ||  // Too Many Requests.
        i == 451 ||  // Unavailable For Legal Reasons.
        i == 500 ||  // Internal Server Error.
        i == 502 ||  // Bad Gateway.
        i == 503 ||  // Service Unavailable.
        i == 504) {  // Gateway Timeout.
      EXPECT_EQ(base::NumberToString(i), http_status_code);
    } else {
      EXPECT_EQ(base::StringPrintf("%dxx", /*http_status_code_class*/ i / 100),
                http_status_code);
    }
  }
}

}  // namespace brave_ads
