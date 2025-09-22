/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_MAYBE_STRIP_WITH_HEADERS_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_MAYBE_STRIP_WITH_HEADERS_H_

#include <type_traits>
#include <variant>

#include "brave/components/endpoint_client/is_request.h"
#include "brave/components/endpoint_client/response.h"
#include "brave/components/endpoint_client/with_headers.h"

namespace endpoint_client::detail {

// Primary template: leaves types unchanged unless
// matched by a partial specialization below.
template <typename T>
struct MaybeStripWithHeadersImpl : std::type_identity<T> {};

// Partial specialization: strips WithHeaders<> if
// the inner T satisfies IsRequest or Response.
template <typename T>
  requires(IsRequest<T> || Response<T>)
struct MaybeStripWithHeadersImpl<WithHeaders<T>>
    : MaybeStripWithHeadersImpl<T> {};

// Partial specialization: strips WithHeaders<> inside a std::variant<> if
// each alternative satisfies Response after stripping.
template <typename... Ts>
  requires(Response<typename MaybeStripWithHeadersImpl<Ts>::type> && ...)
struct MaybeStripWithHeadersImpl<std::variant<Ts...>>
    : std::type_identity<
          std::variant<typename MaybeStripWithHeadersImpl<Ts>::type...>> {};

// Alias: a type with WithHeaders<> removed if
// its MaybeStripWithHeadersImpl specialization applies.
template <typename T>
using MaybeStripWithHeaders = typename MaybeStripWithHeadersImpl<T>::type;

}  // namespace endpoint_client::detail

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_MAYBE_STRIP_WITH_HEADERS_H_
