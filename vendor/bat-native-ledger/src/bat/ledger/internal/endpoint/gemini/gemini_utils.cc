/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/gemini/gemini_utils.h"

#include "base/base64.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/logging/logging.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"

namespace ledger {
namespace endpoint {
namespace gemini {

std::string GetClientId() {
  return ::ledger::gemini::GetClientId();
}

std::string GetClientSecret() {
  return ::ledger::gemini::GetClientSecret();
}

std::vector<std::string> RequestAuthorization(const std::string& token) {
  std::vector<std::string> headers;

  if (!token.empty()) {
    headers.push_back("Authorization: Bearer " + token);
    return headers;
  }

  const std::string id = GetClientId();
  const std::string secret = GetClientSecret();

  std::string user;
  base::Base64Encode(base::StringPrintf("%s:%s", id.c_str(), secret.c_str()),
                     &user);

  headers.push_back("Authorization: Basic " + user);
  return headers;
}

std::string GetApiServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  std::string url;
  if (ledger::_environment == type::Environment::PRODUCTION) {
    url = GEMINI_API_URL;
  } else {
    url = GEMINI_API_STAGING_URL;
  }

  return url + path;
}

std::string GetOauthServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  std::string url;
  if (ledger::_environment == type::Environment::PRODUCTION) {
    url = GEMINI_OAUTH_URL;
  } else {
    url = GEMINI_OAUTH_STAGING_URL;
  }

  return url + path;
}

type::Result CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED ||
      status_code == net::HTTP_FORBIDDEN) {
    return type::Result::EXPIRED_TOKEN;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Account not found");
    return type::Result::NOT_FOUND;
  }

  if (status_code != net::HTTP_OK) {
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger
