/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_H_

#include <concepts>

#include "url/gurl.h"

namespace endpoint_client::detail {

template <typename T>
concept HasURL = requires {
  { T::URL() } -> std::same_as<GURL>;
};

template <typename T>
concept Endpoint = HasURL<T>;

}  // namespace endpoint_client::detail

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_ENDPOINT_H_
