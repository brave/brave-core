/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_BITFLYER_POST_CLAIM_POST_CLAIM_BITFLYER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_BITFLYER_POST_CLAIM_POST_CLAIM_BITFLYER_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST /v3/wallet/bitflyer/{payment_id}/claim
//
// Request body:
// {
//   "linkingInfo": "83b3b77b-e7c3-455b-adda-e476fa0656d2"
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_NOT_FOUND (404)
// HTTP_CONFLICT (409)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {Empty}

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace bitflyer {

using PostClaimBitflyerCallback =
    std::function<void(const type::Result result)>;

class PostClaimBitflyer {
 public:
  explicit PostClaimBitflyer(LedgerImpl* ledger);
  ~PostClaimBitflyer();

  void Request(const std::string& linking_info,
               PostClaimBitflyerCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(const std::string& linking_info);

  type::Result CheckStatusCode(const int status_code);

  void OnRequest(const type::UrlResponse& response,
                 PostClaimBitflyerCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_BITFLYER_POST_CLAIM_POST_CLAIM_BITFLYER_H_
