/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_BODY_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_BODY_H_

#include <concepts>
#include <optional>

#include "brave/components/api_request_helper/api_request_helper.h"

namespace brave_account::endpoint_client::detail {

// Concept that checks whether `T` defines a static, accessible member
// function `FromValue()` such that:
//   - `result.value_body()` can be passed to `T::FromValue()`,
//      where `result` is an `api_request_helper::APIRequestResult`,
//      and that call yields `std::optional<T>`
//
// In short: models any type with a proper static `FromValue()` function
// whose result is a `std::optional<T>`, constructed from the
// `value_body()` of an `api_request_helper::APIRequestResult`.
template <typename T>
concept IsResponseBody = requires(api_request_helper::APIRequestResult result) {
  { T::FromValue(result.value_body()) } -> std::same_as<std::optional<T>>;
};

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_BODY_H_
