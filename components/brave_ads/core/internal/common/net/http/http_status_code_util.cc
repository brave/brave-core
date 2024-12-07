/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code_util.h"

#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "net/http/http_status_code.h"

namespace brave_ads {

namespace {

constexpr auto kAllowedHttpStatusCodes = base::MakeFixedFlatSet<int>({
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

std::optional<std::string> HttpStatusCodeClassToString(
    int http_status_code_class) {
  if (http_status_code_class < 1 || http_status_code_class > 5) {
    // Nonsensical HTTP status code class.
    return std::nullopt;
  }

  return base::StringPrintf("%dxx", http_status_code_class);
}

}  // namespace

bool IsSuccessfulHttpStatusCode(int http_status_code) {
  return http_status_code >= /*200*/ net::HTTP_OK &&
         http_status_code < /*400*/ net::HTTP_BAD_REQUEST;
}

std::optional<std::string> HttpStatusCodeToString(int http_status_code) {
  const int http_status_code_class = http_status_code / 100;

  // Check if the HTTP status code is in the allowed list of codes.
  if (const auto iter = kAllowedHttpStatusCodes.find(http_status_code);
      iter != kAllowedHttpStatusCodes.cend()) {
    // If the HTTP status code is allowed, return it as a string.
    return base::NumberToString(http_status_code);
  }

  // Return a data minimization status code corresponding to the class of the
  // original HTTP status code if the original code is not allowed.
  return HttpStatusCodeClassToString(http_status_code_class);
}

}  // namespace brave_ads
