/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/confirmations.h"

#include "bat/confirmations/internal/ledger_serve_helper.h"
#include "bat/confirmations/internal/static_values.h"

namespace helper {

std::string LedgerServe::GetURL() {
  switch (confirmations::_environment) {
    case ledger::Environment::PRODUCTION: {
      return BAT_LEDGER_PRODUCTION_SERVER;
    }

    case ledger::Environment::STAGING: {
      return BAT_LEDGER_STAGING_SERVER;
    }

    case ledger::Environment::DEVELOPMENT: {
      return BAT_LEDGER_DEVELOPMENT_SERVER;
    }
  }
}

}  // namespace helper
