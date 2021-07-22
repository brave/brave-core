/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CLAIM_GEMINI_POST_CLAIM_GEMINI_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CLAIM_GEMINI_POST_CLAIM_GEMINI_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST /v3/wallet/gemini/{payment_id}/claim
//
// Request body:
// {
//   "linking_info": "mock-linking-info"
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
namespace promotion {

using PostClaimGeminiCallback = std::function<void(const type::Result result)>;

class PostClaimGemini {
 public:
  explicit PostClaimGemini(LedgerImpl* ledger);
  ~PostClaimGemini();

  void Request(const std::string& linking_info,
               const std::string& recipient_id,
               PostClaimGeminiCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(const std::string& linking_info,
                              const std::string& recipient_id);

  type::Result CheckStatusCode(const int status_code);

  void OnRequest(const type::UrlResponse& response,
                 PostClaimGeminiCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CLAIM_GEMINI_POST_CLAIM_GEMINI_H_
