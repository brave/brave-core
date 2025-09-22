/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_METHODS_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_METHODS_H_

#include "brave/components/endpoint_client/is_request.h"
#include "brave/components/endpoint_client/with_method.h"

// Aliases that bind an IsRequestBody to a specific HTTP method.
// These are the only way to form a valid IsRequest type.

namespace endpoint_client {

template <detail::IsRequestBody RequestBody>
using CONNECT = detail::WithMethod<RequestBody, detail::Method::kConnect>;

template <detail::IsRequestBody RequestBody>
using DELETE = detail::WithMethod<RequestBody, detail::Method::kDelete>;

template <detail::IsRequestBody RequestBody>
using GET = detail::WithMethod<RequestBody, detail::Method::kGet>;

template <detail::IsRequestBody RequestBody>
using HEAD = detail::WithMethod<RequestBody, detail::Method::kHead>;

template <detail::IsRequestBody RequestBody>
using OPTIONS = detail::WithMethod<RequestBody, detail::Method::kOptions>;

template <detail::IsRequestBody RequestBody>
using PATCH = detail::WithMethod<RequestBody, detail::Method::kPatch>;

template <detail::IsRequestBody RequestBody>
using POST = detail::WithMethod<RequestBody, detail::Method::kPost>;

template <detail::IsRequestBody RequestBody>
using PUT = detail::WithMethod<RequestBody, detail::Method::kPut>;

template <detail::IsRequestBody RequestBody>
using TRACE = detail::WithMethod<RequestBody, detail::Method::kTrace>;

template <detail::IsRequestBody RequestBody>
using TRACK = detail::WithMethod<RequestBody, detail::Method::kTrack>;

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_METHODS_H_
