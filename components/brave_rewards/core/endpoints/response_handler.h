/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_RESPONSE_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_RESPONSE_HANDLER_H_

#include <utility>

#include "base/functional/callback.h"
#include "base/types/expected.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/endpoints/result_for.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal::endpoints {

template <typename Endpoint>
class ResponseHandler {
 public:
  using Value = typename ResultFor<Endpoint>::Value;
  using Error = typename ResultFor<Endpoint>::Error;
  using Result = base::expected<Value, Error>;

 private:
  static void OnResponse(RewardsEngineImpl& engine,
                         base::OnceCallback<void(Result&&)> callback,
                         mojom::UrlResponsePtr response) {
    DCHECK(response);
    std::move(callback).Run(Endpoint::ProcessResponse(engine, *response));
  }

  // Note that friend class RequestFor<Endpoint>; is not sufficient due to
  // class hierarchies implementing an endpoint (e.g. PostConnect is the one
  // that derives from ResponseHandler<PostConnect>, but we're passing
  // PostConnectBitflyer, PostConnectGemini and PostConnectUphold to
  // RequestFor<>).
  template <typename>
  friend class RequestFor;
};

}  // namespace brave_rewards::internal::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_RESPONSE_HANDLER_H_
