/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/url/url_response_string_util.h"

#include <vector>

#include "base/containers/flat_map.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

namespace ads {

namespace {

std::string HeadersToString(
    const base::flat_map<std::string, std::string>& headers,
    const int indent = 4) {
  std::vector<std::string> formatted_headers;

  const std::string spaces = std::string(' ', indent);

  for (const auto& header : headers) {
    const std::string key = header.first;
    const std::string value = header.second;

    const std::string formatted_header = base::StringPrintf(
        "%s%s: %s", spaces.c_str(), key.c_str(), value.c_str());

    formatted_headers.push_back(formatted_header);
  }

  return base::JoinString(formatted_headers, "\n");
}

}  // namespace

std::string UrlResponseToString(const mojom::UrlResponse& url_response) {
  return base::StringPrintf(
      "URL Response:\n  URL: %s\n  Response "
      "Status Code: %d\n  Response: %s",
      url_response.url.spec().c_str(), url_response.status_code,
      url_response.body.c_str());
}

std::string UrlResponseHeadersToString(const mojom::UrlResponse& url_response) {
  const std::string formatted_headers = HeadersToString(url_response.headers);
  return base::StringPrintf("  Headers:\n%s", formatted_headers.c_str());
}

}  // namespace ads
