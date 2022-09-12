/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CLOBBERED_CLAIMS_POST_CLOBBERED_CLAIMS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CLOBBERED_CLAIMS_POST_CLOBBERED_CLAIMS_H_

#include <string>

#include "base/values.h"
#include "bat/ledger/ledger.h"

// POST /v1/promotions/reportclobberedclaims
//
// Request body:
// {
//   "claimIds": ["asfeq4gerg34gl3g34lg34g"]
// }
//
// Success code:
// HTTP_OK (200)
//
// Error Codes:
// HTTP_BAD_REQUEST (400)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {Empty}

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using PostClobberedClaimsCallback =
    std::function<void(const mojom::Result result)>;

class PostClobberedClaims {
 public:
  explicit PostClobberedClaims(LedgerImpl* ledger);
  ~PostClobberedClaims();

  void Request(base::Value::List corrupted_claims,
               PostClobberedClaimsCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(base::Value::List corrupted_claims);

  mojom::Result CheckStatusCode(const int status_code);

  void OnRequest(const mojom::UrlResponse& response,
                 PostClobberedClaimsCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CLOBBERED_CLAIMS_POST_CLOBBERED_CLAIMS_H_
