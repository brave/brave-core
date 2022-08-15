/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_REQUEST_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_REQUEST_H_

#include <memory>
#include <string>
#include <utility>

#include "base/types/expected.h"
#include "bat/ledger/internal/endpoint/endpoint.h"
#include "bat/ledger/internal/logging/logging.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {

template <typename EndpointImpl>
class Request {
 public:
  Request(const Request&) = delete;
  Request& operator=(const Request&) = delete;

  Request(Request&&) = delete;
  Request& operator=(Request&&) = delete;

  explicit Request(std::unique_ptr<Endpoint> endpoint)
      : endpoint_{(DCHECK(endpoint), std::move(endpoint))},
        request_{endpoint_->Request()} {
    if (!*this) {
      BLOG(0, "Failed to create request!\n" << request_.error());
    }
  }

  explicit operator bool() const { return request_.has_value(); }

  void Send(typename EndpointImpl::Callback callback) && {
    DCHECK(endpoint_);
    DCHECK(request_.has_value() && *request_);

    endpoint_->Send(
        std::move(*request_),
        base::BindOnce(&EndpointImpl::OnResponse, std::move(callback)));
    endpoint_.reset();

    DCHECK(request_.has_value() && !*request_);
    DCHECK(!endpoint_);
  }

 private:
  std::unique_ptr<Endpoint> endpoint_;
  base::expected<type::UrlRequestPtr, std::string> request_;
};

template <typename EndpointImpl>
Request(std::unique_ptr<EndpointImpl>) -> Request<EndpointImpl>;

}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_REQUEST_H_
