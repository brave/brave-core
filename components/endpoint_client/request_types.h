/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_TYPES_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_TYPES_H_

#include "brave/components/endpoint_client/is_request_body.h"
#include "brave/components/endpoint_client/request.h"

namespace endpoint_client {

template <detail::IsRequestBody T>
using Connect = detail::Request<T, detail::Method::kConnect>;

template <detail::IsRequestBody T>
using Delete = detail::Request<T, detail::Method::kDelete>;

template <detail::IsRequestBody T>
using Get = detail::Request<T, detail::Method::kGet>;

template <detail::IsRequestBody T>
using Head = detail::Request<T, detail::Method::kHead>;

template <detail::IsRequestBody T>
using Options = detail::Request<T, detail::Method::kOptions>;

template <detail::IsRequestBody T>
using Patch = detail::Request<T, detail::Method::kPatch>;

template <detail::IsRequestBody T>
using Post = detail::Request<T, detail::Method::kPost>;

template <detail::IsRequestBody T>
using Put = detail::Request<T, detail::Method::kPut>;

template <detail::IsRequestBody T>
using Trace = detail::Request<T, detail::Method::kTrace>;

template <detail::IsRequestBody T>
using Track = detail::Request<T, detail::Method::kTrack>;

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_REQUEST_TYPES_H_
