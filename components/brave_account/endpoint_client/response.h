/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RESPONSE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RESPONSE_H_

#include <optional>

#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/is_response_body.h"
#include "net/base/net_errors.h"

namespace brave_account::endpoint_client {

template <detail::IsResponseBody T, detail::IsResponseBody E>
struct Response {
  using SuccessBody = T;
  using ErrorBody = E;

  int error = net::ERR_IO_PENDING;
  std::optional<int> status;
  std::optional<base::expected<T, E>> body;
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RESPONSE_H_
