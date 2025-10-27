/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_TYPES_H_

#include "brave/components/brave_account/endpoint_client/is_request_body.h"
#include "brave/components/brave_account/endpoint_client/request.h"

namespace brave_account::endpoint_client {

template <detail::IsRequestBody T>
using CONNECT = detail::Request<T, detail::Method::kConnect>;

template <detail::IsRequestBody T>
using DELETE = detail::Request<T, detail::Method::kDelete>;

template <detail::IsRequestBody T>
using GET = detail::Request<T, detail::Method::kGet>;

template <detail::IsRequestBody T>
using HEAD = detail::Request<T, detail::Method::kHead>;

template <detail::IsRequestBody T>
using OPTIONS = detail::Request<T, detail::Method::kOptions>;

template <detail::IsRequestBody T>
using PATCH = detail::Request<T, detail::Method::kPatch>;

template <detail::IsRequestBody T>
using POST = detail::Request<T, detail::Method::kPost>;

template <detail::IsRequestBody T>
using PUT = detail::Request<T, detail::Method::kPut>;

template <detail::IsRequestBody T>
using TRACE = detail::Request<T, detail::Method::kTrace>;

template <detail::IsRequestBody T>
using TRACK = detail::Request<T, detail::Method::kTrack>;

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_TYPES_H_
