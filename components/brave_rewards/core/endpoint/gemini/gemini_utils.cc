/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/gemini_utils.h"

#include "base/base64.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/gemini/gemini_util.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::core {
namespace endpoint {
namespace gemini {

std::string GetClientId() {
  return core::gemini::GetClientId();
}

std::string GetClientSecret() {
  return core::gemini::GetClientSecret();
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
  if (_environment == mojom::Environment::PRODUCTION) {
    url = BUILDFLAG(GEMINI_API_URL);
  } else {
    url = BUILDFLAG(GEMINI_API_STAGING_URL);
  }

  return url + path;
}

std::string GetOauthServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  std::string url;
  if (_environment == mojom::Environment::PRODUCTION) {
    url = BUILDFLAG(GEMINI_OAUTH_URL);
  } else {
    url = BUILDFLAG(GEMINI_OAUTH_STAGING_URL);
  }

  return url + path;
}

mojom::Result CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED ||
      status_code == net::HTTP_FORBIDDEN) {
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Account not found");
    return mojom::Result::NOT_FOUND;
  }

  if (status_code != net::HTTP_OK) {
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace brave_rewards::core
