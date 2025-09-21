/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_MAYBE_STRIP_WITH_HEADERS_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_MAYBE_STRIP_WITH_HEADERS_H_

#include <type_traits>
#include <variant>

#include "brave/components/endpoint_client/request.h"
#include "brave/components/endpoint_client/response.h"
#include "brave/components/endpoint_client/with_headers.h"

namespace endpoints::detail {

template <typename>
struct MaybeStripWithHeadersImpl;

template <typename T>
  requires(detail::Request<T> || detail::Response<T>)
struct MaybeStripWithHeadersImpl<T> : std::type_identity<T> {};

template <typename T>
struct MaybeStripWithHeadersImpl<WithHeaders<T>>
    : MaybeStripWithHeadersImpl<T> {};

// This partial participates only if each alternative
// becomes a detail::Response after stripping.
template <typename... Ts>
  requires(detail::Response<typename MaybeStripWithHeadersImpl<Ts>::type> &&
           ...)
struct MaybeStripWithHeadersImpl<std::variant<Ts...>>
    : std::type_identity<
          std::variant<typename MaybeStripWithHeadersImpl<Ts>::type...>> {};

template <typename T>
using MaybeStripWithHeaders =
    typename detail::MaybeStripWithHeadersImpl<T>::type;

}  // namespace endpoints::detail

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_MAYBE_STRIP_WITH_HEADERS_H_
