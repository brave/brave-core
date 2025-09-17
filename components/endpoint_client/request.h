/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_

#include <type_traits>

#include "base/json/json_writer.h"
#include "brave/components/endpoint_client/with_method.h"

namespace endpoints {

template <typename>
struct WithHeaders;

namespace detail {

template <typename T>
concept RequestBody = requires(T t) { base::WriteJson(t.ToValue()); } &&
                      std::is_member_function_pointer_v<decltype(&T::ToValue)>;

template <typename>
struct RequestImpl : std::false_type {};

template <typename T, Method M>
struct RequestImpl<WithMethod<T, M>> : std::bool_constant<RequestBody<T>> {};

template <typename T>
struct RequestImpl<WithHeaders<T>> : RequestImpl<T> {};

template <typename T>
concept Request = RequestImpl<T>::value;

}  // namespace detail
}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_
