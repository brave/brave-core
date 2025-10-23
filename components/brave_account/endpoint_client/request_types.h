/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_TYPES_H_

#include "brave/components/brave_account/endpoint_client/is_request_body.h"
#include "brave/components/brave_account/endpoint_client/request.h"

namespace brave_account::endpoint_client {

template <detail::IsRequestBody RequestBody>
using CONNECT = detail::Request<RequestBody, detail::Method::kConnect>;

template <detail::IsRequestBody RequestBody>
using DELETE = detail::Request<RequestBody, detail::Method::kDelete>;

template <detail::IsRequestBody RequestBody>
using GET = detail::Request<RequestBody, detail::Method::kGet>;

template <detail::IsRequestBody RequestBody>
using HEAD = detail::Request<RequestBody, detail::Method::kHead>;

template <detail::IsRequestBody RequestBody>
using OPTIONS = detail::Request<RequestBody, detail::Method::kOptions>;

template <detail::IsRequestBody RequestBody>
using PATCH = detail::Request<RequestBody, detail::Method::kPatch>;

template <detail::IsRequestBody RequestBody>
using POST = detail::Request<RequestBody, detail::Method::kPost>;

template <detail::IsRequestBody RequestBody>
using PUT = detail::Request<RequestBody, detail::Method::kPut>;

template <detail::IsRequestBody RequestBody>
using TRACE = detail::Request<RequestBody, detail::Method::kTrace>;

template <detail::IsRequestBody RequestBody>
using TRACK = detail::Request<RequestBody, detail::Method::kTrack>;

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_REQUEST_TYPES_H_
