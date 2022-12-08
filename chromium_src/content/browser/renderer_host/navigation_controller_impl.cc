/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/navigation_controller_impl.h"

#include "brave/net/query_filter/query_filter.h"

// This is for browser-initiated navigations (e.g. typing a URL in the URL bar).
#define BRAVE_NAVIGATION_CONTROLLER_IMPL_CREATE_NAVIGATION_REQUEST_FROM_LOAD_PARAMS \
  net::query_filter::MaybeRemoveTrackingQueryParameters(                            \
      params.initiator_origin, url_to_load);

#include "src/content/browser/renderer_host/navigation_controller_impl.cc"

#undef BRAVE_NAVIGATION_CONTROLLER_IMPL_CREATE_NAVIGATION_REQUEST_FROM_LOAD_PARAMS
