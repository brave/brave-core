/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_TYPES_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_TYPES_H_

#include "brave/components/endpoint_client/is_request_body.h"
#include "brave/components/endpoint_client/request.h"

namespace endpoint_client {

// GET and HEAD requests don't take a body: RFC 9110 gives a body no defined
// meaning for them, and `SimpleURLLoader` `DCHECK`s against attaching one.
using Get = internal::Request<internal::Method::kGet>;
using Head = internal::Request<internal::Method::kHead>;

template <internal::IsRequestBody... RequestBody>
using Connect = internal::Request<internal::Method::kConnect, RequestBody...>;

template <internal::IsRequestBody... RequestBody>
using Delete = internal::Request<internal::Method::kDelete, RequestBody...>;

template <internal::IsRequestBody... RequestBody>
using Options = internal::Request<internal::Method::kOptions, RequestBody...>;

template <internal::IsRequestBody... RequestBody>
using Patch = internal::Request<internal::Method::kPatch, RequestBody...>;

template <internal::IsRequestBody... RequestBody>
using Post = internal::Request<internal::Method::kPost, RequestBody...>;

template <internal::IsRequestBody... RequestBody>
using Put = internal::Request<internal::Method::kPut, RequestBody...>;

template <internal::IsRequestBody... RequestBody>
using Trace = internal::Request<internal::Method::kTrace, RequestBody...>;

template <internal::IsRequestBody... RequestBody>
using Track = internal::Request<internal::Method::kTrack, RequestBody...>;

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_TYPES_H_
