/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_STREAM_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_STREAM_H_

#include "brave/components/endpoint_client/is_response_body.h"
#include "brave/components/endpoint_client/stream.h"

namespace endpoint_client::internal {

template <typename>
inline constexpr bool kIsStreamWithoutErrorBody = false;

// A stream without an error body is a Stream<> with no body type.
template <>
inline constexpr bool kIsStreamWithoutErrorBody<Stream<>> = true;

template <typename Stream>
concept IsStreamWithoutErrorBody = kIsStreamWithoutErrorBody<Stream>;

template <typename>
inline constexpr bool kIsStreamWithErrorBody = false;

// A stream with an error body is a Stream<> with an error response body.
template <IsResponseBody ErrorBody>
inline constexpr bool kIsStreamWithErrorBody<Stream<ErrorBody>> = true;

template <typename Stream>
concept IsStreamWithErrorBody = kIsStreamWithErrorBody<Stream>;

// A stream is either a stream without an error body or a stream with one.
template <typename Stream>
concept IsStream =
    IsStreamWithoutErrorBody<Stream> || IsStreamWithErrorBody<Stream>;

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_IS_STREAM_H_
