/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_POST_SAFETYNET_POST_SAFETYNET_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_POST_SAFETYNET_POST_SAFETYNET_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST /v2/attestations/safetynet
//
// Request body:
// {
//   "paymentIds": [
//     "83b3b77b-e7c3-455b-adda-e476fa0656d2"
//   ]
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "nonce": "c4645786-052f-402f-8593-56af2f7a21ce"
// }

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {
namespace promotion {

using PostSafetynetCallback = std::function<void(
    const ledger::Result result,
    const std::string& nonce)>;

class PostSafetynet {
 public:
  explicit PostSafetynet(bat_ledger::LedgerImpl* ledger);
  ~PostSafetynet();

  void Request(PostSafetynetCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload();

  ledger::Result CheckStatusCode(const int status_code);

  ledger::Result ParseBody(
      const std::string& body,
      std::string* nonce);

  void OnRequest(
      const ledger::UrlResponse& response,
      PostSafetynetCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_POST_SAFETYNET_POST_SAFETYNET_H_
