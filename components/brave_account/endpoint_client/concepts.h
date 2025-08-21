/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CONCEPTS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CONCEPTS_H_

#include <concepts>
#include <optional>
#include <string_view>

#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "url/gurl.h"

namespace brave_account::endpoint_client::concepts {

namespace detail {
template <typename T>
concept ToValue = requires(T t) {
  { t.ToValue() } -> std::convertible_to<base::ValueView>;
} && std::is_member_function_pointer_v<decltype(&T::ToValue)>;

template <typename T>
concept FromValue = requires(api_request_helper::APIRequestResult result) {
  { T::FromValue(result.value_body()) } -> std::same_as<std::optional<T>>;
};

template <typename T>
concept URL = requires {
  { T::URL() } -> std::same_as<GURL>;
};

template <typename T>
concept Method = requires {
  { T::Method() } -> std::convertible_to<std::string_view>;
};
}  // namespace detail

template <typename T>
concept Request = detail::ToValue<T>;

template <typename T>
concept Response = detail::FromValue<T>;

template <typename T>
concept Error = detail::FromValue<T>;

template <typename T>
concept Endpoint =
    requires {
      typename T::Request;
      typename T::Response;
      typename T::Error;
    } && Request<typename T::Request> && Response<typename T::Response> &&
    Error<typename T::Error> && detail::URL<T> && detail::Method<T>;

}  // namespace brave_account::endpoint_client::concepts

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CONCEPTS_H_
