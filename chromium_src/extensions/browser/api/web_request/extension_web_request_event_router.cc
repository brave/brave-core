/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_EXTENSION_WEB_REQUEST_EVENT_ROUTER_ON_AUTH_REQUIRED      \
  if (browser_context) {                                               \
    ClearSignaled(browser_context, request->id, kOnBeforeSendHeaders); \
    ClearSignaled(browser_context, request->id, kOnSendHeaders);       \
    ClearSignaled(browser_context, request->id, kOnHeadersReceived);   \
  }

#include "src/extensions/browser/api/web_request/extension_web_request_event_router.cc"

#undef BRAVE_EXTENSION_WEB_REQUEST_EVENT_ROUTER_ON_AUTH_REQUIRED
