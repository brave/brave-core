/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_RESPONSE_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_RESPONSE_H_

#include <concepts>
#include <optional>
#include <type_traits>
#include <variant>

#include "base/values.h"

namespace endpoints::detail {

// Concept: a type T is a ResponseBody if
// it defines static std::optional<T> FromValue(const base::Value&).
template <typename T>
concept ResponseBody = requires(const base::Value& value) {
  { T::FromValue(value) } -> std::same_as<std::optional<T>>;
};

// Primary template: a type is not a Response unless
// matched by a partial specialization below.
template <typename>
struct ResponseImpl : std::false_type {};

// Partial specialization: a type is a Response if
// it satisfies ResponseBody.
template <ResponseBody Body>
struct ResponseImpl<Body> : std::true_type {};

// Partial specialization: a std::variant<> is a Response if
// - it has at least two alternatives, and
// - every alternative satisfies ResponseBody.
template <ResponseBody... Bodies>
  requires(sizeof...(Bodies) >= 2)
struct ResponseImpl<std::variant<Bodies...>> : std::true_type {};

// Concept: a type is a Response if
// its ResponseImpl specialization evaluates to true.
template <typename T>
concept Response = ResponseImpl<T>::value;

}  // namespace endpoints::detail

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_RESPONSE_H_
