/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/request/request_util.h"

namespace braveledger_request_util {

std::string BuildUrl(
    const std::string& path,
    const std::string& prefix,
    const ServerTypes& server) {
  std::string url;
  switch (server) {
    case ServerTypes::BALANCE: {
      switch (ledger::_environment) {
        case ledger::Environment::STAGING:
          url = BALANCE_STAGING_SERVER;
          break;
        case ledger::Environment::PRODUCTION:
          url = BALANCE_PRODUCTION_SERVER;
          break;
        case ledger::Environment::DEVELOPMENT:
          url = BALANCE_DEVELOPMENT_SERVER;
          break;
      }
      break;
    }
    case ServerTypes::PUBLISHER: {
      switch (ledger::_environment) {
        case ledger::Environment::STAGING:
          url = PUBLISHER_STAGING_SERVER;
          break;
        case ledger::Environment::PRODUCTION:
          url = PUBLISHER_PRODUCTION_SERVER;
          break;
        case ledger::Environment::DEVELOPMENT:
          url = PUBLISHER_DEVELOPMENT_SERVER;
          break;
      }
      break;
    }
    case ServerTypes::PUBLISHER_DISTRO: {
      switch (ledger::_environment) {
        case ledger::Environment::STAGING:
          url = PUBLISHER_DISTRO_STAGING_SERVER;
          break;
        case ledger::Environment::PRODUCTION:
          url = PUBLISHER_DISTRO_PRODUCTION_SERVER;
          break;
        case ledger::Environment::DEVELOPMENT:
          url = PUBLISHER_DISTRO_DEVELOPMENT_SERVER;
          break;
      }
      break;
    }
    case ServerTypes::LEDGER: {
      switch (ledger::_environment) {
        case ledger::Environment::STAGING:
          url = LEDGER_STAGING_SERVER;
          break;
        case ledger::Environment::PRODUCTION:
          url = LEDGER_PRODUCTION_SERVER;
          break;
        case ledger::Environment::DEVELOPMENT:
          url = LEDGER_DEVELOPMENT_SERVER;
          break;
      }
      break;
    }
  }

  if (url.empty()) {
    DCHECK(false);
    return "";
  }

  return url + prefix + path;
}

}  // namespace braveledger_request_util
