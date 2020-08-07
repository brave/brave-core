/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/logging/logging_util.h"

#include <sstream>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/logging.h"

namespace ledger {

bool ShouldLogHeader(const std::string& header) {
  const std::vector<std::string> allowed_headers {
      "digest",
      "signature",
      "accept",
      "content-type"
  };

  for (const auto& item : allowed_headers) {
    if (base::StartsWith(header, item, base::CompareCase::INSENSITIVE_ASCII)) {
      return true;
    }
  }

  return false;
}

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
    if (!ShouldLogHeader(header)) {
      continue;
    }

    log += base::StringPrintf("\n> Header %s", header.c_str());
  }

  return log;
}

std::string UrlResponseToString(
    const char* func,
    const ledger::UrlResponse& response) {
  std::string result;
  if (!response.error.empty()) {
    result = "Error (" + response.error + ")";
  } else if (response.status_code >= 200 && response.status_code < 300) {
    result = "Success";
  } else {
    result = "Failure";
  }

  std::string formatted_headers;
  for (const auto& header : response.headers) {
    formatted_headers +=
        "\n> Header " + header.first + ": " + header.second;
  }

  return base::StringPrintf(
      "\n[ RESPONSE - %s ]\n"
      "> Url: %s\n"
      "> Result: %s\n"
      "> HTTP Code: %d\n"
      "> Body: %s"
      "%s",
      func,
      response.url.c_str(),
      result.c_str(),
      response.status_code,
      response.body.c_str(),
      formatted_headers.c_str());
}

void LogUrlResponse(
    const char* func,
    const ledger::UrlResponse& response,
    const bool long_response) {
  std::string result;
  if (!response.error.empty()) {
    result = "Error (" + response.error + ")";
  } else if (response.status_code >= 200 && response.status_code < 300) {
    result = "Success";
  } else {
    result = "Failure";
  }

  std::string formatted_headers;
  for (const auto& header : response.headers) {
    formatted_headers +=
        "\n> Header " + header.first + ": " + header.second;
  }

  const std::string response_basic = base::StringPrintf(
      "\n[ RESPONSE - %s ]\n"
      "> Url: %s\n"
      "> Result: %s\n"
      "> HTTP Code: %d\n"
      "> Body: %s",
      func,
      response.url.c_str(),
      result.c_str(),
      response.status_code,
      response.body.c_str());

  const std::string response_headers = base::StringPrintf(
      "\n[ RESPONSE HEADERS ]\n"
      "> Url: %s\n"
      "%s",
      response.url.c_str(),
      formatted_headers.c_str());

  BLOG(long_response ? 7 : 6, response_basic);
  BLOG(9, response_headers);
}

}  // namespace ledger
