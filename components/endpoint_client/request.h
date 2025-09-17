/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_

#include <concepts>
#include <string_view>
#include <type_traits>

#include "base/json/json_writer.h"
#include "net/http/http_request_headers.h"

namespace endpoints {

template <typename>
struct WithHeaders;

namespace detail {

template <typename T>
concept HasToValue = requires(T t) { base::WriteJson(t.ToValue()); } &&
                     std::is_member_function_pointer_v<decltype(&T::ToValue)>;

template <typename T>
concept HasMethod = requires {
  { T::Method() } -> std::same_as<std::string_view>;
};

template <typename T>
struct RequestImpl {
  static constexpr bool value = HasToValue<T> && HasMethod<T>;
};

template <typename T>
struct RequestImpl<WithHeaders<T>> {
  static constexpr bool value = RequestImpl<T>::value;
};

template <typename T>
concept Request = RequestImpl<T>::value;

}  // namespace detail
}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_H_
