/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_GEMINI_POST_COMMIT_TRANSACTION_POST_COMMIT_TRANSACTION_GEMINI_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_GEMINI_POST_COMMIT_TRANSACTION_POST_COMMIT_TRANSACTION_GEMINI_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/endpoints/post_commit_transaction/post_commit_transaction.h"
#include "bat/ledger/internal/endpoints/response_handler.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_endpoints.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// POST /v1/payments/pay
//
// Request body:
//
// Response body:
// {
//   "amount": 0.95,
//   "currency": "BAT",
//   "destination": "621e9ca3-6c64-4055-bce7-e3460841a7cc",
//   "result": "OK",
//   "status": "Pending",
//   "tx_ref": "c40ccc6a-8579-6435-90be-66ea7ea96c1b"
// }

namespace ledger {
class LedgerImpl;

namespace endpoints {

class PostCommitTransactionGemini;

template <>
struct ResultFor<PostCommitTransactionGemini> {
  using Value = void;
  using Error = mojom::PostCommitTransactionGeminiError;
};

class PostCommitTransactionGemini final
    : public PostCommitTransaction,
      public ResponseHandler<PostCommitTransactionGemini> {
 public:
  PostCommitTransactionGemini(LedgerImpl*,
                              std::string&& token,
                              std::string&& address,
                              std::string&& transaction_id,
                              std::string&& destination,
                              double amount);

  static Result ProcessResponse(const mojom::UrlResponse&);

 private:
  absl::optional<std::string> Url() const override;
  absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;
  std::string ContentType() const override;

  std::string destination_;
  double amount_;
};

}  // namespace endpoints
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_GEMINI_POST_COMMIT_TRANSACTION_POST_COMMIT_TRANSACTION_GEMINI_H_
