/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/logging_util.h"

#include <time.h>

#include <sstream>

#include "base/strings/stringprintf.h"

namespace ledger {

std::string UrlRequestToString(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const UrlMethod method) {
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
  std::string formatted_headers;
  for (auto header = headers.begin(); header != headers.end(); ++header) {
    formatted_headers += "> headers\n" + header->first + ": " + header->second;
    if (header != headers.end()) {
      formatted_headers += "\n";
    }
  }
  return formatted_headers;
}

std::string UrlResponseToString(
    const char* func,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  const std::string result =
      response_status_code >= 200 && response_status_code < 300
          ? "Success" : "Failure";

  const std::string timestamp = std::to_string(std::time(nullptr));

  const std::string formatted_headers = HeadersToString(headers);

  return base::StringPrintf("[ RESPONSE - %s ]\n> time: %s\n> result: %s\n"
      "> http code: %d\n> response: %s\n%s[ END RESPONSE ]", func,
          timestamp.c_str(), result.c_str(), response_status_code,
              response.c_str(), formatted_headers.c_str());
}

}  // namespace ledger
