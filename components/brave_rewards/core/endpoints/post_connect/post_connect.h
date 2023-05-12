/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_POST_CONNECT_POST_CONNECT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_POST_CONNECT_POST_CONNECT_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/ledger_endpoints.mojom.h"
#include "brave/components/brave_rewards/core/endpoints/request_builder.h"
#include "brave/components/brave_rewards/core/endpoints/response_handler.h"
#include "brave/components/brave_rewards/core/endpoints/result_for.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/mojom_structs.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_rewards::internal::endpoints {

class PostConnect;

template <>
struct ResultFor<PostConnect> {
  using Value = void;
  using Error = mojom::PostConnectError;
};

class PostConnect : public RequestBuilder, public ResponseHandler<PostConnect> {
 public:
  static Result ProcessResponse(const mojom::UrlResponse&);
  static ConnectExternalWalletResult ToConnectExternalWalletResult(
      const Result&);

 protected:
  virtual const char* Path() const = 0;

 private:
  absl::optional<std::string> Url() const override;
  absl::optional<std::vector<std::string>> Headers(
      const std::string& content) const override;
  std::string ContentType() const override;
};

}  // namespace brave_rewards::internal::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_POST_CONNECT_POST_CONNECT_H_
