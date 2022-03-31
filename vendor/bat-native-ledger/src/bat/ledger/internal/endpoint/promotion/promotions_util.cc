/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"

#include "bat/ledger/ledger.h"

namespace ledger {
namespace endpoint {
namespace promotion {

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  std::string url;
  switch (ledger::_environment) {
    case type::Environment::DEVELOPMENT:
      url = REWARDS_GRANT_DEV_ENDPOINT;
      break;
    case type::Environment::STAGING:
      url = REWARDS_GRANT_STAGING_ENDPOINT;
      break;
    case type::Environment::PRODUCTION:
      url = REWARDS_GRANT_PROD_ENDPOINT;
      break;
  }

  return url + path;
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
