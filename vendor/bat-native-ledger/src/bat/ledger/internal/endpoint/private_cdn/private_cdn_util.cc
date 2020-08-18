/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/private_cdn/private_cdn_util.h"

#include "bat/ledger/ledger.h"

namespace ledger {
namespace endpoint {
namespace private_cdn {

const char kDevelopment[] = "https://pcdn.brave.software";
const char kStaging[] = "https://pcdn.bravesoftware.com";
const char kProduction[] = "https://pcdn.brave.com";

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  std::string url;
  switch (ledger::_environment) {
    case ledger::Environment::DEVELOPMENT:
      url = kDevelopment;
      break;
    case ledger::Environment::STAGING:
      url = kStaging;
      break;
    case ledger::Environment::PRODUCTION:
      url = kProduction;
      break;
  }

  return url + path;
}

}  // namespace private_cdn
}  // namespace endpoint
}  // namespace ledger
