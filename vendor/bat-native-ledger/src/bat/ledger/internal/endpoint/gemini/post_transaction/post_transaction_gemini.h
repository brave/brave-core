/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_TRANSACTION_POST_TRANSACTION_GEMINI_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_TRANSACTION_POST_TRANSACTION_GEMINI_H_

#include <string>

#include "bat/ledger/internal/gemini/gemini.h"
#include "bat/ledger/ledger.h"

// POST https://api.gemini.com/v1/payments/pay
// Headers:
//   Authorization: Bearer w43gqn3EYg9BxkMt4LmhvVfTi7jzwALq73SBA5FwnkVS
//   X-GEMINI-PAYLOAD: base64-payload
//
// Request body:
// {}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)
// HTTP_NOT_FOUND (404)
//
// Response body:
// {
//   "result": "OK",
//   "tx_ref": "A5721BF3-530C-42AF-8DEE-005DCFF76970",
//   "amount": 1,
//   "currency": "BAT",
//   "destination": "60bf98d6-d1f8-4d35-8650-8d4570a86b60",
//   "status": "Completed",
//   "timestamp": 1623171893237
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace gemini {

using PostTransactionCallback =
    std::function<void(const type::Result result, const std::string& id)>;

class PostTransaction {
 public:
  explicit PostTransaction(LedgerImpl* ledger);
  ~PostTransaction();

  void Request(const std::string& token,
               const ::ledger::gemini::Transaction& transaction,
               PostTransactionCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(const ::ledger::gemini::Transaction& transaction);

  type::Result ParseBody(const std::string& body,
                         std::string* transfer_id,
                         std::string* transfer_status);

  void OnRequest(const type::UrlResponse& response,
                 PostTransactionCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_TRANSACTION_POST_TRANSACTION_GEMINI_H_
