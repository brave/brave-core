/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_BODY_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_BODY_H_

#include <concepts>
#include <type_traits>

#include "base/values.h"

namespace brave_account::endpoint_client::detail {

// Concept that checks whether `T` defines a non-static, accessible member
// function `ToValue()` such that:
//   - `t.ToValue()` is a valid expression,
//      and that call yields a `base::Value::Dict`
//   - `&T::ToValue` is a valid member function pointer (ensures it's a
//      non-static member function)
//
// In short: models any type with a proper non-static `ToValue()` function
// whose result is a `base::Value::Dict`.
template <typename T>
concept IsRequestBody = requires(T t) {
  { t.ToValue() } -> std::same_as<base::Value::Dict>;
} && std::is_member_function_pointer_v<decltype(&T::ToValue)>;

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_BODY_H_
