/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_PATCH_WALLETS_PATCH_WALLETS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_PATCH_WALLETS_PATCH_WALLETS_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/ledger_endpoints.mojom.h"
#include "brave/components/brave_rewards/core/endpoints/request_builder.h"
#include "brave/components/brave_rewards/core/endpoints/response_handler.h"
#include "brave/components/brave_rewards/core/endpoints/result_for.h"
#include "brave/components/brave_rewards/core/mojom_structs.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// PATCH /v4/wallets/<rewards_payment_id>
//
// Request body:
// {
//   "geo_country": "US"
// }
//
// Response body: -

namespace brave_rewards::internal::endpoints {

class PatchWallets;

template <>
struct ResultFor<PatchWallets> {
  using Value = void;
  using Error = mojom::PatchWalletsError;
};

class PatchWallets final : public RequestBuilder,
                           public ResponseHandler<PatchWallets> {
 public:
  static Result ProcessResponse(const mojom::UrlResponse&);

  explicit PatchWallets(std::string&& geo_country);

 private:
  const char* Path() const;

  absl::optional<std::string> Url() const override;
  mojom::UrlMethod Method() const override;
  absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;
  absl::optional<std::string> Content() const override;
  std::string ContentType() const override;

  std::string geo_country_;
};

}  // namespace brave_rewards::internal::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_PATCH_WALLETS_PATCH_WALLETS_H_
