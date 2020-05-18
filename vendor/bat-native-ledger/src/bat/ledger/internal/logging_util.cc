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
  std::string log = "\n[ REQUEST ]";
  log += base::StringPrintf("\n> URL: %s", url.c_str());

  std::stringstream ss;
  ss << method;
  log += base::StringPrintf("\n> Method: %s", ss.str().c_str());

  if (!content.empty()) {
    log += base::StringPrintf("\n> Content: %s", content.c_str());
  }

  if (!content_type.empty()) {
    log += base::StringPrintf("\n> Content Type: %s", content_type.c_str());
  }

  for (const auto& header : headers) {
    log += base::StringPrintf("\n> Header %s", header.c_str());
  }

  return log;
}

std::string UrlResponseToString(
    const char* func,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  const std::string result =
      response_status_code >= 200 && response_status_code < 300
          ? "Success" : "Failure";

  std::string formatted_headers;
  for (const auto& header : headers) {
    formatted_headers +=
        "\n> Header " + header.first + ": " + header.second;
  }

  return base::StringPrintf(
      "\n[ RESPONSE - %s ]\n"
      "> Result: %s\n"
      "> HTTP Code: %d\n"
      "> Body: %s"
      "%s",
      func,
      result.c_str(),
      response_status_code,
      response.c_str(),
      formatted_headers.c_str());
}

}  // namespace ledger
