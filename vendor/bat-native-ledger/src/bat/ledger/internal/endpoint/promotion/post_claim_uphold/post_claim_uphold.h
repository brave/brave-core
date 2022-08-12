/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CLAIM_UPHOLD_POST_CLAIM_UPHOLD_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CLAIM_UPHOLD_POST_CLAIM_UPHOLD_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST /v3/wallet/uphold/{payment_id}/claim
//
// Request body:
// {
//   "signedLinkingRequest": "......",
//   "anonymousAddress": "asfeq4gerg34gl3g34lg34g"
// }
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

using PostClaimUpholdCallback = base::OnceCallback<void(type::Result)>;

class PostClaimUphold {
 public:
  explicit PostClaimUphold(LedgerImpl* ledger);

  ~PostClaimUphold();

  void Request(const std::string& address,
               PostClaimUpholdCallback callback) const;

 private:
  std::string GeneratePayload(const std::string& address) const;

  std::string GetUrl() const;

  void OnRequest(PostClaimUpholdCallback callback,
                 const type::UrlResponse& response) const;

  type::Result ProcessResponse(const type::UrlResponse& response) const;

  type::Result ParseBody(const std::string& body) const;

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CLAIM_UPHOLD_POST_CLAIM_UPHOLD_H_
