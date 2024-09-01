/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"

#include <vector>

#include "base/containers/flat_map.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {

std::string HeadersToString(
    const base::flat_map<std::string, std::string>& headers,
    const int indent = 4) {
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
  return base::StringPrintf(
      "URL Response:\n  URL: %s\n  Response "
      "Status Code: %d\n  Response: %s",
      mojom_url_response.url.spec().c_str(), mojom_url_response.status_code,
      mojom_url_response.body.c_str());
}

std::string UrlResponseHeadersToString(
    const mojom::UrlResponseInfo& mojom_url_response) {
  return base::StrCat(
      {"  Headers:\n", HeadersToString(mojom_url_response.headers)});
}

}  // namespace brave_ads
