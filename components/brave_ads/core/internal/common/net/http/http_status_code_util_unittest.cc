/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code_util.h"

#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsHttpStatusCodeUtilTest, HttpStatusCodeToString) {
  // Arrange
  static constexpr auto kAllowedHttpStatusCodes = base::MakeFixedFlatSet<int>({
      400,  // Bad Request.
      401,  // Unauthorized.
      403,  // Forbidden.
      404,  // Not Found.
      407,  // Proxy Authentication Required.
      408,  // Request Timeout.
      429,  // Too Many Requests.
      451,  // Unavailable For Legal Reasons.
      500,  // Internal Server Error.
      502,  // Bad Gateway.
      503,  // Service Unavailable.
      504   // Gateway Timeout.
  });

  // Act & Assert
  for (int i = 0; i <= net::HTTP_STATUS_CODE_MAX; ++i) {
    const std::optional<std::string> http_status_code =
        HttpStatusCodeToString(i);
    if (!http_status_code) {
      // Nonsensical HTTP status code.
      continue;
    }

    if (kAllowedHttpStatusCodes.contains(i)) {
      EXPECT_EQ(base::NumberToString(i), http_status_code);
    } else {
      EXPECT_EQ(base::StringPrintf("%dxx", /*http_status_code_class*/ i / 100),
                http_status_code);
    }
  }
}

}  // namespace brave_ads
