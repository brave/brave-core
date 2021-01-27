/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_utils.h"

#include "base/base64.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/ledger.h"

namespace ledger {
namespace endpoint {
namespace bitflyer {

const char kUrlStaging[] = BITFLYER_STAGING_URL;
const char kUrlProduction[] = "https://bitflyer.jp";

std::string GetClientSecret() {
  return BITFLYER_CLIENT_SECRET;
}

std::vector<std::string> RequestAuthorization(const std::string& token) {
  std::vector<std::string> headers;

  if (!token.empty()) {
    headers.push_back("Authorization: Bearer " + token);
    return headers;
  }

  const std::string id = ::ledger::bitflyer::GetClientId();
  const std::string secret = GetClientSecret();

  std::string user;
  base::Base64Encode(base::StringPrintf("%s:%s", id.c_str(), secret.c_str()),
                     &user);

  headers.push_back("Authorization: Basic " + user);

  return headers;
}

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  std::string url;
  if (ledger::_environment == type::Environment::PRODUCTION) {
    url = kUrlProduction;
  } else {
    url = kUrlStaging;
  }

  return url + path;
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger
