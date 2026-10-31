/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_HAS_TO_VALUE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_HAS_TO_VALUE_H_

#include <concepts>
#include <type_traits>

#include "base/values.h"

namespace brave_account::endpoint_client::detail {

// A type with a non-static ToValue()
// callable on a const object and returning base::DictValue.
template <typename T>
concept HasToValue = requires(const T& t) {
  { t.ToValue() } -> std::same_as<base::DictValue>;
} && std::is_member_function_pointer_v<decltype(&T::ToValue)>;

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_HAS_TO_VALUE_H_
