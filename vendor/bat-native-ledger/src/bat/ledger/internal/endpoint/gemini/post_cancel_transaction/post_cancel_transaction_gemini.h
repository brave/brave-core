/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_CANCEL_TRANSACTION_POST_CANCEL_TRANSACTION_GEMINI_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_CANCEL_TRANSACTION_POST_CANCEL_TRANSACTION_GEMINI_H_

#include <string>

#include "bat/ledger/ledger.h"

// GET https://api.gemini.com/v1/payment/cancel
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_UNAUTHORIZED (401)

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace gemini {

using PostCancelTransactionCallback =
    std::function<void(const type::Result result)>;

class PostCancelTransaction {
 public:
  explicit PostCancelTransaction(LedgerImpl* ledger);
  ~PostCancelTransaction();

  void Request(const std::string& token,
               const std::string& tx_ref,
               PostCancelTransactionCallback callback);

 private:
  void OnRequest(const type::UrlResponse& response,
                 PostCancelTransactionCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace gemini
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_GEMINI_POST_CANCEL_TRANSACTION_POST_CANCEL_TRANSACTION_GEMINI_H_
