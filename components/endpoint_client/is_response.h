/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_RESPONSE_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_RESPONSE_H_

#include <concepts>
#include <optional>
#include <variant>

#include "base/values.h"

namespace endpoint_client::detail {

// Concept: a type T satisfies IsResponseBody if
// it defines static std::optional<T> FromValue(const base::Value&).
template <typename T>
concept IsResponseBody = requires(const base::Value& value) {
  { T::FromValue(value) } -> std::same_as<std::optional<T>>;
};

// Primary template: a type does not satisfies IsResponse unless
// matched by a partial specialization below.
template <typename>
inline constexpr bool kIsResponse = false;

// Partial specialization: a type satisfies IsResponse if
// it satisfies IsResponseBody.
template <IsResponseBody ResponseBody>
inline constexpr bool kIsResponse<ResponseBody> = true;

// Partial specialization: a std::variant<> satisfies IsResponse if
// - it has at least two alternatives, and
// - every alternative satisfies IsResponseBody.
template <IsResponseBody... ResponseBodies>
  requires(sizeof...(ResponseBodies) >= 2)
inline constexpr bool kIsResponse<std::variant<ResponseBodies...>> = true;

// Concept: a type satisfies IsResponse if
// its kIsResponse specialization evaluates to true.
template <typename T>
concept IsResponse = kIsResponse<T>;

}  // namespace endpoint_client::detail

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_RESPONSE_H_
