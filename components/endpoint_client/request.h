/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_

#include <type_traits>

#include "base/json/json_writer.h"

namespace endpoints::detail {

// Concept: a type is a RequestBody if
// - it defines a non-static member function ToValue(),
// - whose result can be passed to base::WriteJson().
template <typename T>
concept RequestBody = requires(T t) { base::WriteJson(t.ToValue()); } &&
                      std::is_member_function_pointer_v<decltype(&T::ToValue)>;

// Forward declarations.
enum class Method;
template <RequestBody, Method>
struct WithMethod;

// Primary template: a type is not a Request unless
// matched by the partial specialization below.
template <typename>
struct RequestImpl : std::false_type {};

// Partial specialization: WithMethod<Body, M> is a Request if
// Body satisfies RequestBody.
template <RequestBody Body, Method M>
struct RequestImpl<WithMethod<Body, M>> : std::true_type {};

// Concept: a type is a Request if
// its RequestImpl specialization evaluates to true.
template <typename T>
concept Request = RequestImpl<T>::value;

}  // namespace endpoints::detail

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_
