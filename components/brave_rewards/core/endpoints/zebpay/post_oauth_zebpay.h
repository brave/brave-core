/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_ZEBPAY_POST_OAUTH_ZEBPAY_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_ZEBPAY_POST_OAUTH_ZEBPAY_H_

#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"
#include "brave/components/brave_rewards/core/endpoints/request_builder.h"
#include "brave/components/brave_rewards/core/endpoints/response_handler.h"
#include "brave/components/brave_rewards/core/endpoints/result_for.h"

// POST /connect/token
//
// Request body:
// grant_type=authorization_code
// &redirect_uri=rewards://zebpay/authorization
// &code=9fc12dc7a9931ef1f9de5c087c0ccfcd0e9bea06a6ddacc84c45e14c07aa83c9

// clang-format off
// Response body:
// {
//   "access_token": "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
//   "expires_in": 43200,
//   "id_token": "...",
//   "linking_info": "...",
//   "token_type": "Bearer"
// }
// clang-format on

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoints {

class PostOAuthZebPay;

template <>
struct ResultFor<PostOAuthZebPay> {
  using Value = std::tuple<std::string,   // access token
                           std::string,   // linking info
                           std::string>;  // deposit ID

  using Error = mojom::PostOAuthZebPayError;
};

class PostOAuthZebPay final : public RequestBuilder,
                              public ResponseHandler<PostOAuthZebPay> {
 public:
  static Result ProcessResponse(RewardsEngineImpl& engine,
                                const mojom::UrlResponse&);

  PostOAuthZebPay(RewardsEngineImpl& engine, const std::string& code);
  ~PostOAuthZebPay() override;

 private:
  std::optional<std::string> Url() const override;
  std::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;
  std::optional<std::string> Content() const override;
  std::string ContentType() const override;
  bool SkipLog() const override;

  std::string code_;
};

}  // namespace endpoints
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_ZEBPAY_POST_OAUTH_ZEBPAY_H_
