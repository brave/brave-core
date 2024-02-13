/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_UPHOLD_POST_OAUTH_UPHOLD_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_UPHOLD_POST_OAUTH_UPHOLD_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"
#include "brave/components/brave_rewards/core/endpoints/request_builder.h"
#include "brave/components/brave_rewards/core/endpoints/response_handler.h"
#include "brave/components/brave_rewards/core/endpoints/result_for.h"

// POST /oauth2/token
//
// Request body:
// code=bb50f9d4782fb86a4302ef18179033abb17c257f&grant_type=authorization_code
//
// clang-format off
// Response body:
// {
//   "access_token": "9fd84e43c803622cc65a63c6d380a47612d7d718",
//   "expires_in": 7775999,
//   "scope": "cards:read cards:write user:read transactions:transfer:application transactions:transfer:others",  // NOLINT
//   "token_type": "bearer"
// }
// clang-format on

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoints {

class PostOAuthUphold;

template <>
struct ResultFor<PostOAuthUphold> {
  using Value = std::string;  // access token
  using Error = mojom::PostOAuthUpholdError;
};

class PostOAuthUphold final : public RequestBuilder,
                              public ResponseHandler<PostOAuthUphold> {
 public:
  static Result ProcessResponse(RewardsEngineImpl& engine,
                                const mojom::UrlResponse&);

  PostOAuthUphold(RewardsEngineImpl& engine, const std::string& code);
  ~PostOAuthUphold() override;

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

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_UPHOLD_POST_OAUTH_UPHOLD_H_
