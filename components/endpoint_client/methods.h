/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_METHODS_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_METHODS_H_

#include "brave/components/endpoint_client/request.h"
#include "brave/components/endpoint_client/with_method.h"

// Aliases that bind a RequestBody to a specific HTTP method.
// These are the only way to form a valid Request type.

namespace endpoints {

template <detail::RequestBody Body>
using CONNECT = detail::WithMethod<Body, detail::Method::kConnect>;

template <detail::RequestBody Body>
using DELETE = detail::WithMethod<Body, detail::Method::kDelete>;

template <detail::RequestBody Body>
using GET = detail::WithMethod<Body, detail::Method::kGet>;

template <detail::RequestBody Body>
using HEAD = detail::WithMethod<Body, detail::Method::kHead>;

template <detail::RequestBody Body>
using OPTIONS = detail::WithMethod<Body, detail::Method::kOptions>;

template <detail::RequestBody Body>
using PATCH = detail::WithMethod<Body, detail::Method::kPatch>;

template <detail::RequestBody Body>
using POST = detail::WithMethod<Body, detail::Method::kPost>;

template <detail::RequestBody Body>
using PUT = detail::WithMethod<Body, detail::Method::kPut>;

template <detail::RequestBody Body>
using TRACE = detail::WithMethod<Body, detail::Method::kTrace>;

template <detail::RequestBody Body>
using TRACK = detail::WithMethod<Body, detail::Method::kTrack>;

}  // namespace endpoints

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_METHODS_H_
