/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/confirmations.h"

#include "bat/confirmations/internal/grant_rewards_serve_helper.h"
#include "bat/confirmations/internal/static_values.h"

namespace helper {

std::string GrantRewardsServe::GetURL() {
  switch (confirmations::_environment) {
    case ledger::Environment::PRODUCTION: {
      return PROMOTION_PRODUCTION_SERVER;
    }

    case ledger::Environment::STAGING: {
      return PROMOTION_STAGING_SERVER;
    }

    case ledger::Environment::DEVELOPMENT: {
      return PROMOTION_DEVELOPMENT_SERVER;
    }
  }
}

}  // namespace helper
