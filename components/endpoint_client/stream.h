/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_STREAM_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_STREAM_H_

#include <optional>
#include <string_view>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/endpoint_client/is_response_body.h"
#include "net/base/net_errors.h"

namespace net {
class HttpResponseHeaders;
}  // namespace net

namespace endpoint_client {

namespace internal {

// The callbacks of a streaming request, which completes with a `Result`.
template <typename Result>
struct StreamCallbacks {
  // Runs once the response headers arrive, whatever the status. Optional.
  base::OnceCallback<void(scoped_refptr<net::HttpResponseHeaders> headers)>
      on_response_started;
  // Runs for each chunk of a 2xx body. `chunk` is only valid during the call.
  base::RepeatingCallback<void(std::string_view chunk)> on_chunk;
  // Runs once the stream has ended, whether it succeeded or not.
  base::OnceCallback<void(Result result)> on_complete;
};

}  // namespace internal

template <typename...>
struct Stream {
  static_assert(false,
                "Stream<> takes either no body type or an error body type!");
};

// A streamed response without an error body. A 2xx body is delivered in
// chunks, and any other body is discarded.
template <>
struct Stream<> {
  struct Result {
    int net_error = net::ERR_IO_PENDING;
    std::optional<int> status_code;
  };

  using Callbacks = internal::StreamCallbacks<Result>;
};

// A streamed response with an error body. A 2xx body is delivered in chunks,
// and any other body is deserialized into the error body.
template <internal::IsResponseBody ErrorResponseBody>
struct Stream<ErrorResponseBody> {
  using ErrorBody = ErrorResponseBody;

  struct Result {
    int net_error = net::ERR_IO_PENDING;
    std::optional<int> status_code;
    // std::nullopt for a 2xx response, or if deserialization fails.
    std::optional<ErrorBody> error_body;
  };

  using Callbacks = internal::StreamCallbacks<Result>;
};

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_STREAM_H_
