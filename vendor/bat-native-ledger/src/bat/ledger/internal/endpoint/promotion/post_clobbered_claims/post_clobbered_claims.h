/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_POST_CLOBBERED_\
CLAIMS_POST_CLOBBERED_CLAIMS_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_POST_CLOBBERED_\
CLAIMS_POST_CLOBBERED_CLAIMS_H_

#include <string>

#include "base/values.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {
namespace promotion {

using PostClobberedClaimsCallback = std::function<void(
    const ledger::Result result)>;

class PostClobberedClaims {
 public:
  explicit PostClobberedClaims(bat_ledger::LedgerImpl* ledger);
  ~PostClobberedClaims();

  void Request(
    base::Value corrupted_claims,
    PostClobberedClaimsCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(base::Value corrupted_claims);

  ledger::Result CheckStatusCode(const int status_code);

  void OnRequest(
      const ledger::UrlResponse& response,
      PostClobberedClaimsCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_POST_CLOBBERED_\
// CLAIMS_POST_CLOBBERED_CLAIMS_H_
