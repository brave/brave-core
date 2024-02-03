/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_REQUEST_FOR_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_REQUEST_FOR_H_

#include <optional>
#include <type_traits>
#include <utility>

#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/common/callback_helpers.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/endpoints/request_builder.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal::endpoints {

template <typename, typename = void>
inline constexpr bool enumerator_check = false;

template <typename T>
inline constexpr bool
    enumerator_check<T, std::void_t<decltype(T::kFailedToCreateRequest)>> =
        true;

template <typename Endpoint>
class RequestFor {
 public:
  template <typename... Ts>
  RequestFor(RewardsEngineImpl& engine, Ts&&... ts)
      : engine_(engine),
        request_(Endpoint(engine, std::forward<Ts>(ts)...).Request()) {
    static_assert(std::is_base_of_v<RequestBuilder, Endpoint>,
                  "Endpoint should be derived from RequestBuilder!");
  }

  RequestFor(const RequestFor&) = delete;
  RequestFor& operator=(const RequestFor&) = delete;

  RequestFor(RequestFor&&) = delete;
  RequestFor& operator=(RequestFor&&) = delete;

  void Send(base::OnceCallback<void(typename Endpoint::Result&&)> callback) && {
    if (!request_ || !*request_) {
      BLOG(0, "Failed to create request!");

      static_assert(enumerator_check<typename Endpoint::Error>,
                    "Please make sure the error type of your endpoint has the "
                    "kFailedToCreateRequest enumerator!");

      DeferCallback(FROM_HERE, std::move(callback),
                    base::unexpected(Endpoint::Error::kFailedToCreateRequest));
      return;
    }

    URLLoader::LogLevel log_level = (*request_)->skip_log
                                        ? URLLoader::LogLevel::kNone
                                        : URLLoader::LogLevel::kDetailed;

    engine_->Get<URLLoader>().Load(
        std::move(*request_), log_level,
        base::BindOnce(&Endpoint::OnResponse, std::move(callback)));
  }

 private:
  const raw_ref<RewardsEngineImpl> engine_;
  std::optional<mojom::UrlRequestPtr> request_;
};

}  // namespace brave_rewards::internal::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_REQUEST_FOR_H_
