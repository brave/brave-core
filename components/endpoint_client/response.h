/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_RESPONSE_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_RESPONSE_H_

#include <concepts>
#include <optional>
#include <type_traits>

#include "base/values.h"

namespace endpoints {

template <typename>
struct WithHeaders;

namespace detail {

template <typename T>
concept ResponseBody = requires(const base::Value& value) {
  { T::FromValue(value) } -> std::same_as<std::optional<T>>;
};

template <typename T>
struct ResponseImpl : std::bool_constant<ResponseBody<T>> {};

template <typename T>
struct ResponseImpl<WithHeaders<T>> : ResponseImpl<T> {};

template <typename T>
concept Response = ResponseImpl<T>::value;

}  // namespace detail
}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_RESPONSE_H_
