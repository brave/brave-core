/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"

#include "base/base64.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/ledger.h"

namespace ledger {
namespace endpoint {
namespace uphold {

const char kStaging[] = "https://api-sandbox.uphold.com";
const char kProduction[] = "https://api.uphold.com";

std::string GetClientId() {
  return ::ledger::uphold::GetClientId();
}

std::string GetClientSecret() {
  return ::ledger::uphold::GetClientSecret();
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
  base::Base64Encode(
      base::StringPrintf(
          "%s:%s",
          id.c_str(),
          secret.c_str()),
      &user);

  headers.push_back("Authorization: Basic " + user);

  return headers;
}

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  std::string url;
  if (ledger::_environment == type::Environment::PRODUCTION) {
    url = kProduction;
  } else {
    url = kStaging;
  }

  return url + path;
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
