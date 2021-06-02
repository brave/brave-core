/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_DELETE_CLAIM_DELETE_CLAIM_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_DELETE_CLAIM_DELETE_CLAIM_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST /v3/wallet/{custodian}/{payment_id}/claim
//
// Request body:
// {}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_FORBIDDEN (403)
// HTTP_NOT_FOUND (404)
// HTTP_CONFLICT (409)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {Empty}

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using DeleteClaimCallback = std::function<void(const type::Result result)>;

class DeleteClaim {
 public:
  explicit DeleteClaim(LedgerImpl* ledger);
  ~DeleteClaim();

  void Request(const std::string& custodian, DeleteClaimCallback callback);

 private:
  std::string GetUrl(const std::string& custodian);

  type::Result CheckStatusCode(const int status_code);

  void OnRequest(const type::UrlResponse& response,
                 DeleteClaimCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_DELETE_CLAIM_DELETE_CLAIM_H_
