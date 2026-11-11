/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_TYPES_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_TYPES_H_

#include "brave/components/endpoint_client/is_request_body.h"
#include "brave/components/endpoint_client/request.h"

namespace endpoint_client {

template <internal::IsRequestBody T>
using Connect = internal::Request<T, internal::Method::kConnect>;

template <internal::IsRequestBody T>
using Delete = internal::Request<T, internal::Method::kDelete>;

template <internal::IsRequestBody T>
using Get = internal::Request<T, internal::Method::kGet>;

template <internal::IsRequestBody T>
using Head = internal::Request<T, internal::Method::kHead>;

template <internal::IsRequestBody T>
using Options = internal::Request<T, internal::Method::kOptions>;

template <internal::IsRequestBody T>
using Patch = internal::Request<T, internal::Method::kPatch>;

template <internal::IsRequestBody T>
using Post = internal::Request<T, internal::Method::kPost>;

template <internal::IsRequestBody T>
using Put = internal::Request<T, internal::Method::kPut>;

template <internal::IsRequestBody T>
using Trace = internal::Request<T, internal::Method::kTrace>;

template <internal::IsRequestBody T>
using Track = internal::Request<T, internal::Method::kTrack>;

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_TYPES_H_
