/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_REQUEST_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_REQUEST_H_

#include <type_traits>

#include "base/json/json_writer.h"

namespace endpoint_client::detail {

// Concept: a type satisfies IsRequestBody if
// - it defines a non-static member function ToValue(),
// - whose result can be passed to base::WriteJson().
template <typename T>
concept IsRequestBody = requires(T t) {
  base::WriteJson(t.ToValue());
} && std::is_member_function_pointer_v<decltype(&T::ToValue)>;

// Forward declarations.
enum class Method;
template <IsRequestBody, Method>
struct WithMethod;

// Primary template: a type does not satisfy IsRequest unless
// matched by the partial specialization below.
template <typename>
inline constexpr bool kIsRequest = false;

// Partial specialization: WithMethod<Body, M> satisfies IsRequest if
// Body satisfies IsRequestBody.
template <IsRequestBody Body, Method M>
inline constexpr bool kIsRequest<WithMethod<Body, M>> = true;

// Concept: a type satisfies IsRequest if
// its kIsRequest specialization evaluates to true.
template <typename T>
concept IsRequest = kIsRequest<T>;

}  // namespace endpoint_client::detail

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_REQUEST_H_
