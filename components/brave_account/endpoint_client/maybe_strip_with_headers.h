/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_MAYBE_STRIP_WITH_HEADERS_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_MAYBE_STRIP_WITH_HEADERS_H_

#include <type_traits>

#include "brave/components/brave_account/endpoint_client/is_request.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"

namespace brave_account::endpoint_client::detail {

// Primary template: leaves types unchanged unless
// matched by the partial specialization below.
template <typename T>
struct MaybeStripWithHeadersImpl : std::type_identity<T> {};

// Partial specialization: strips WithHeaders<> if
// the inner T satisfies IsRequest.
template <IsRequest T>
struct MaybeStripWithHeadersImpl<WithHeaders<T>> : std::type_identity<T> {};

// Alias: a type with WithHeaders<> removed if
// its MaybeStripWithHeadersImpl specialization applies.
template <typename T>
using MaybeStripWithHeaders = typename MaybeStripWithHeadersImpl<T>::type;

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_MAYBE_STRIP_WITH_HEADERS_H_
