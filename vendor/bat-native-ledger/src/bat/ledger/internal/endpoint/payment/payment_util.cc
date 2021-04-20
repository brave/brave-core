/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/payment/payment_util.h"

#include "base/command_line.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/option_keys.h"

namespace ledger {
namespace endpoint {
namespace payment {

const char kDevelopment[] = "https://payment.rewards.brave.software";
const char kStaging[] = "https://payment.rewards.bravesoftware.com";
const char kProduction[] = "https://payment.rewards.brave.com";

std::string GetServerUrl(LedgerImpl* ledger, const std::string& path) {
  DCHECK(!path.empty());

  std::string url = ledger->ledger_client()->GetStringOption(
      ledger::option::kPaymentServiceURL);

  if (url.empty()) {
    switch (ledger::_environment) {
      case type::Environment::DEVELOPMENT:
        url = kDevelopment;
        break;
      case type::Environment::STAGING:
        url = kStaging;
        break;
      case type::Environment::PRODUCTION:
        url = kProduction;
        break;
    }
  }

  return url + path;
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
