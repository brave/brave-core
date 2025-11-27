/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"

#include <vector>

#include "base/containers/flat_map.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_ads {

namespace {

std::string HeadersToString(
    const base::flat_map<std::string, std::string>& headers,
    const size_t indent = 4) {
  std::vector<std::string> formatted_headers;
  formatted_headers.reserve(headers.size());

  const std::string spaces(indent, ' ');

  for (const auto& [header, value] : headers) {
    formatted_headers.push_back(base::ReplaceStringPlaceholders(
        "$1$2: $3", {spaces, header, value}, nullptr));
  }

  return base::JoinString(formatted_headers, "\n");
}

}  // namespace

std::string UrlResponseToString(
    const mojom::UrlResponseInfo& mojom_url_response) {
  return absl::StrFormat(
      "URL Response:\n  URL: %s\n  Response Code: %d\n  Response: %s",
      mojom_url_response.url.spec(), mojom_url_response.code,
      mojom_url_response.body);
}

std::string UrlResponseHeadersToString(
    const mojom::UrlResponseInfo& mojom_url_response) {
  return "  Headers:\n" + HeadersToString(mojom_url_response.headers);
}

}  // namespace brave_ads
