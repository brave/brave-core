/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_PATCH_WALLETS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_PATCH_WALLETS_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/rewards_endpoints.mojom.h"
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

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoints {

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

  PatchWallets(RewardsEngineImpl& engine, std::string&& geo_country);
  ~PatchWallets() override;

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

}  // namespace endpoints
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_PATCH_WALLETS_H_
