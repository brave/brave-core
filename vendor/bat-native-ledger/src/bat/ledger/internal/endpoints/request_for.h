/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_REQUEST_FOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_REQUEST_FOR_H_

#include <type_traits>
#include <utility>

#include "base/callback.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "bat/ledger/internal/endpoints/request_builder.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/logging.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger::endpoints {

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
  RequestFor(LedgerImpl* ledger, Ts&&... ts)
      : ledger_(ledger),
        request_(Endpoint(ledger, std::forward<Ts>(ts)...).Request()) {
    static_assert(std::is_base_of_v<RequestBuilder, Endpoint>,
                  "Endpoint should be derived from RequestBuilder!");
    DCHECK(ledger_);
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

      base::SequencedTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::BindOnce(
              std::move(callback),
              base::unexpected(Endpoint::Error::kFailedToCreateRequest)));
      return;
    }

    SendImpl(
        std::tuple(ledger_, std::move(*request_), base::Seconds(0),
                   base::BindOnce(&Endpoint::OnResponse, std::move(callback))));
  }

 private:
  static void SendImpl(
      absl::optional<std::tuple<LedgerImpl*,
                                mojom::UrlRequestPtr,
                                base::TimeDelta,
                                client::LoadURLCallback>> params) {
    if (params) {
      auto [ledger, request, delta, callback] = std::move(*params);

      auto load_url_callback =
          base::BindOnce(
              [](LedgerImpl* led, mojom::UrlRequestPtr req,
                 client::LoadURLCallback cb,
                 const mojom::UrlResponse& response) -> decltype(params) {
                if (/*response.status_code == net::HTTP_TOO_MANY_REQUESTS &&*/
                    req->retry_on_rate_limiting--) {
                  BLOG(0, "req->retry_on_rate_limiting--");
                  ledger::LogUrlResponse(__func__, response);

                  return std::tuple(led, std::move(req), base::Seconds(5),
                                    std::move(cb));
                } else {
                  std::move(cb).Run(response);
                  return absl::nullopt;
                }
              },
              ledger, request->Clone(), std::move(callback))
              .Then(base::BindOnce(&SendImpl));

      if (delta.is_positive()) {
        base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
            FROM_HERE,
            base::BindOnce(static_cast<void (LedgerImpl::*)(
                               mojom::UrlRequestPtr, client::LoadURLCallback)>(
                               &LedgerImpl::LoadURL),
                           base::Unretained(ledger), std::move(request),
                           std::move(load_url_callback)),
            std::move(delta));
      } else {
        ledger->LoadURL(std::move(request), std::move(load_url_callback));
      }
    }
  }

  LedgerImpl* ledger_;  // NOT OWNED
  absl::optional<ledger::mojom::UrlRequestPtr> request_;
};

}  // namespace ledger::endpoints

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINTS_REQUEST_FOR_H_
