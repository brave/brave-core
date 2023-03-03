/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_BITFLYER_POST_COMMIT_TRANSACTION_POST_COMMIT_TRANSACTION_BITFLYER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_BITFLYER_POST_COMMIT_TRANSACTION_POST_COMMIT_TRANSACTION_BITFLYER_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/endpoints/post_commit_transaction/post_commit_transaction.h"
#include "bat/ledger/internal/endpoints/response_handler.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/public/interfaces/ledger_endpoints.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// POST /api/link/v1/coin/withdraw-to-deposit-id/request
//
// Request body:
// {
//   "amount": "0.950000",
//   "currency_code": "BAT",
//   "deposit_id": "b3149e8b-0001-4588-a243-ed792d445469",
//   "dry_run": false,
//   "transfer_id": "72a46abc-0683-4716-a1ba-52dc130b3dba"
// }
//
// Response body:
// {
//   "amount": 0.95,
//   "currency_code": "BAT",
//   "dry_run": false,
//   "message": null,
//   "transfer_id": "72a46abc-0683-4716-a1ba-52dc130b3dba",
//   "transfer_status": "SUCCESS"
// }

namespace ledger::endpoints {

class PostCommitTransactionBitFlyer;

template <>
struct ResultFor<PostCommitTransactionBitFlyer> {
  using Value = void;
  using Error = mojom::PostCommitTransactionBitFlyerError;
};

class PostCommitTransactionBitFlyer final
    : public PostCommitTransaction,
      public ResponseHandler<PostCommitTransactionBitFlyer> {
 public:
  using PostCommitTransaction::PostCommitTransaction;

  static Result ProcessResponse(const mojom::UrlResponse&);

 private:
  absl::optional<std::string> Url() const override;
  absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;
  absl::optional<std::string> Content() const override;
  std::string ContentType() const override;
};

}  // namespace ledger::endpoints

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_BITFLYER_POST_COMMIT_TRANSACTION_POST_COMMIT_TRANSACTION_BITFLYER_H_
