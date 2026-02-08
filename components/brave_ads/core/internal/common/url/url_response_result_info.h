/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_RESPONSE_RESULT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_RESPONSE_RESULT_INFO_H_

#include <string>
#include <variant>

namespace brave_ads {

template <typename T>
struct UrlResponseSuccess {
  T value;
};

template <>
struct UrlResponseSuccess<void> {};

struct UrlResponseError {
  std::string message;
  bool should_retry = false;
};

// Represents either a successful result (carrying a value, or no value for
// void) or an error describing why the operation failed.
template <typename T>
using UrlResponseResultInfo =
    std::variant<UrlResponseSuccess<T>, UrlResponseError>;

template <typename T>
[[nodiscard]] inline constexpr const T* GetValue(
    const UrlResponseResultInfo<T>& result) noexcept {
  if (const auto* success = std::get_if<UrlResponseSuccess<T>>(&result)) {
    return &success->value;
  }
  return nullptr;
}

template <typename T>
[[nodiscard]] inline constexpr const UrlResponseError* GetError(
    const UrlResponseResultInfo<T>& result) noexcept {
  return std::get_if<UrlResponseError>(&result);
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_RESPONSE_RESULT_INFO_H_
