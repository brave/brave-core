/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_REQUEST_REQUEST_FOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_REQUEST_REQUEST_FOR_H_

#include <string>
#include <type_traits>
#include <utility>

#include "base/callback.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/logging.h"
#include "bat/ledger/internal/request/request_builder.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger::request {

template <typename ConcreteRequestBuilder>
class RequestFor {
 public:
  template <typename... Ts>
  RequestFor(LedgerImpl* ledger, Ts&&... ts)
      : ledger_(ledger),
        request_(
            ConcreteRequestBuilder{ledger, std::forward<Ts>(ts)...}.Request()) {
    static_assert(
        std::is_base_of_v<RequestBuilder, ConcreteRequestBuilder>,
        "ConcreteRequestBuilder should be derived from RequestBuilder!");
    DCHECK(ledger_);

    if (!*this) {
      BLOG(0, "Failed to create request!");
    }
  }

  RequestFor(const RequestFor&) = delete;
  RequestFor& operator=(const RequestFor&) = delete;

  RequestFor(RequestFor&&) = delete;
  RequestFor& operator=(RequestFor&&) = delete;

  explicit operator bool() const { return request_.has_value(); }

  void Send(typename ConcreteRequestBuilder::Callback callback) && {
    DCHECK(*this && *request_);

    ledger_->LoadURL(std::move(*request_),
                     base::BindOnce(&ConcreteRequestBuilder::OnResponse,
                                    std::move(callback)));
  }

 private:
  LedgerImpl* ledger_;  // NOT OWNED
  absl::optional<type::UrlRequestPtr> request_;
};

}  // namespace ledger::request

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_REQUEST_REQUEST_FOR_H_
