/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BITFLYER_GET_TRANSACTION_STATUS_GET_TRANSACTION_STATUS_BITFLYER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BITFLYER_GET_TRANSACTION_STATUS_GET_TRANSACTION_STATUS_BITFLYER_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/ledger_endpoints.mojom.h"
#include "brave/components/brave_rewards/core/endpoints/get_transaction_status/get_transaction_status.h"
#include "brave/components/brave_rewards/core/endpoints/response_handler.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// GET /api/link/v1/coin/withdraw-to-deposit-id/status
//
// Request body:
// {
//   "transfer_id": "3e4b73ef-70dc-45bf-b154-f2f32e72a61a"
// }
//
// Response body:
// TODO(sszaloki)

namespace brave_rewards::internal::endpoints {

class GetTransactionStatusBitFlyer;

template <>
struct ResultFor<GetTransactionStatusBitFlyer> {
  using Value = void;  // transaction completed
  using Error = mojom::GetTransactionStatusBitFlyerError;
};

class GetTransactionStatusBitFlyer final
    : public GetTransactionStatus,
      public ResponseHandler<GetTransactionStatusBitFlyer> {
 public:
  using GetTransactionStatus::GetTransactionStatus;

  static Result ProcessResponse(const mojom::UrlResponse&);

 private:
  absl::optional<std::string> Url() const override;
  absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;
  absl::optional<std::string> Content() const override;
  std::string ContentType() const override;
};

}  // namespace brave_rewards::internal::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BITFLYER_GET_TRANSACTION_STATUS_GET_TRANSACTION_STATUS_BITFLYER_H_
