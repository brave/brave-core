/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_GET_TRANSACTION_GET_TRANSACTION_GEMINI_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_GET_TRANSACTION_GET_TRANSACTION_GEMINI_H_

#include <string>

#include "bat/ledger/internal/gemini/gemini.h"
#include "bat/ledger/ledger.h"

// GET https://api.gemini.com/v1/payments/<client_id>/<tx_ref>
// Headers:
//   Authorization: Bearer ***
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
//   "reason": ""
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace gemini {

using GetTransactionCallback = std::function<void(const type::Result result)>;

class GetTransaction {
 public:
  explicit GetTransaction(LedgerImpl* ledger);
  ~GetTransaction();

  void Request(const std::string& token,
               const std::string& tx_ref,
               GetTransactionCallback callback);

 private:
  std::string GetUrl(const std::string& tx_ref);

  type::Result ParseBody(const std::string& body, std::string* status);

  void OnRequest(const type::UrlResponse& response,
                 GetTransactionCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_GET_TRANSACTION_GET_TRANSACTION_GEMINI_H_
