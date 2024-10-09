/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CONNECT_ZEBPAY_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CONNECT_ZEBPAY_H_

#include <optional>
#include <string>

#include "brave/components/brave_rewards/core/endpoints/common/post_connect.h"

// POST /v3/wallet/zebpay/{rewards_payment_id}/claim
//
// clang-format off
// Request body:
// {
//   "linkingInfo": "..."
// }
// clang-format on
//
// Response body:
// {
//   "geoCountry": "IN"
// }

namespace brave_rewards::internal {
class RewardsEngine;

namespace endpoints {

class PostConnectZebPay final : public PostConnect {
 public:
  PostConnectZebPay(RewardsEngine& engine, std::string&& linking_info);
  ~PostConnectZebPay() override;

 private:
  std::optional<std::string> Content() const override;

  std::string Path(base::cstring_view payment_id) const override;

  std::string linking_info_;
};

}  // namespace endpoints
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_POST_CONNECT_ZEBPAY_H_
