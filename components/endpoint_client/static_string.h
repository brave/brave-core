/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_STATIC_STRING_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_STATIC_STRING_H_

#include <algorithm>
#include <cstddef>

namespace brave_account::endpoint_client::detail {

template <std::size_t N>
struct StaticString {
  // Non-explicit constructor allows string literals to be passed
  // directly as template arguments: BraveEndpoint<"prefix", "/path", ...>.
  // NOLINTNEXTLINE(google-explicit-constructor)
  consteval StaticString(const char (&array)[N]) {
    std::copy_n(array, N, value);
  }

  char value[N]{};
};

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_STATIC_STRING_H_
