/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/logging_util.h"

#include <sstream>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

namespace confirmations {

namespace {

}  // namespace

std::string UrlRequestToString(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const URLRequestMethod method) {
  std::string log = "URL Request:\n";

  log += base::StringPrintf("  URL: %s\n", url.c_str());

  if (!headers.empty()) {
    log += "  Headers:\n";

    for (const auto& header : headers) {
      log += base::StringPrintf("    %s\n", header.c_str());
    }
  }

  if (!content.empty()) {
    log += base::StringPrintf("  Content: %s\n", content.c_str());
  }

  if (!content_type.empty()) {
    log += base::StringPrintf("  Content Type: %s\n", content_type.c_str());
  }

  std::stringstream ss;
  ss << method;

  log += base::StringPrintf("  Method: %s", ss.str().c_str());

  return log;
}

std::string HeadersToString(
    const std::map<std::string, std::string>& headers) {
  std::vector<std::string> formatted_headers;

  for (auto& header : headers) {
    const std::string key = header.first;
    const std::string value = header.second;

    const std::string formatted_header =
        base::StringPrintf("    %s: %s", key.c_str(), value.c_str());

    formatted_headers.push_back(formatted_header);
  }

  return base::JoinString(formatted_headers, "\n");
}

std::string UrlResponseToString(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  const std::string formatted_headers = HeadersToString(headers);

  return base::StringPrintf("URL Response:\n  URL: %s\n  Response "
      "Status Code: %d\n  Response: %s\n  Headers:\n%s", url.c_str(),
          response_status_code, response.c_str(), formatted_headers.c_str());
}

}  // namespace confirmations
