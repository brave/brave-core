/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/logging_util.h"

#include <vector>

#include "base/containers/flat_map.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/url_util.h"

namespace ads {

namespace {

bool ShouldAllowHeader(const std::string& header) {
  const std::vector<std::string> allowed_headers{"digest", "signature",
                                                 "accept", "content-type"};

  for (const auto& allowed_header : allowed_headers) {
    if (base::StartsWith(header, allowed_header,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      return true;
    }
  }

  return false;
}

std::string HeadersToString(const std::vector<std::string>& headers,
                            const int indent = 4) {
  std::vector<std::string> formatted_headers;

  const std::string spaces = std::string(' ', indent);

  for (const auto& header : headers) {
    if (!ShouldAllowHeader(header)) {
      continue;
    }

    const std::string formatted_header =
        base::StringPrintf("%s%s", spaces.c_str(), header.c_str());

    formatted_headers.push_back(formatted_header);
  }

  return base::JoinString(formatted_headers, "\n");
}

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

std::string UrlRequestToString(const mojom::UrlRequestPtr& url_request) {
  std::string log = "URL Request:\n";

  log += base::StringPrintf("  URL: %s\n", url_request->url.c_str());

  if (!url_request->content.empty()) {
    log += base::StringPrintf("  Content: %s\n", url_request->content.c_str());
  }

  if (!url_request->content_type.empty()) {
    log += base::StringPrintf("  Content Type: %s\n",
                              url_request->content_type.c_str());
  }

  std::ostringstream ss;
  ss << url_request->method;

  log += base::StringPrintf("  Method: %s", ss.str().c_str());

  return log;
}

std::string UrlRequestHeadersToString(const mojom::UrlRequestPtr& url_request) {
  std::string log = "  Headers:\n";

  if (!url_request->headers.empty()) {
    log += HeadersToString(url_request->headers);
  }

  return log;
}

std::string UrlResponseToString(const mojom::UrlResponse& url_response) {
  return base::StringPrintf(
      "URL Response:\n  URL: %s\n  Response "
      "Status Code: %d\n  Response: %s",
      url_response.url.c_str(), url_response.status_code,
      url_response.body.c_str());
}

std::string UrlResponseHeadersToString(const mojom::UrlResponse& url_response) {
  const std::string formatted_headers = HeadersToString(url_response.headers);

  return base::StringPrintf("  Headers:\n%s", formatted_headers.c_str());
}

}  // namespace ads
