/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/payment/payment_util.h"

#include "bat/ledger/ledger.h"

namespace ledger {
namespace endpoint {
namespace payment {

const char kDevelopment[] = "https://payment.rewards.brave.software";
const char kStaging[] = "https://payment.rewards.bravesoftware.com";
const char kProduction[] = "https://payment.rewards.brave.com";

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());

  std::string url;
  switch (ledger::_environment) {
    case mojom::Environment::DEVELOPMENT:
      url = kDevelopment;
      break;
    case mojom::Environment::STAGING:
      url = kStaging;
      break;
    case mojom::Environment::PRODUCTION:
      url = kProduction;
      break;
  }

  return url + path;
}

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger
