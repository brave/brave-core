/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/confirmations.h"

#include "bat/confirmations/internal/ledger_serve_helper.h"
#include "bat/confirmations/internal/static_values.h"

namespace helper {

std::string LedgerServe::GetURL() {
  if (confirmations::_is_production) {
    return BAT_LEDGER_PRODUCTION_SERVER;
  } else {
    return BAT_LEDGER_STAGING_SERVER;
  }
}

}  // namespace helper
