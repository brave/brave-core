// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "net/url_request/url_request_context_builder.h"

// Allow unit tests to include chrome-untrusted:// iframes
#define SetProtocolHandler(scheme, handler) \
  SetProtocolHandler(scheme, handler);      \
  context_builder->SetProtocolHandler(      \
      "chrome-untrusted",                   \
      web::URLDataManagerIOSBackend::CreateProtocolHandler(browser_state))

#include <ios/web/public/test/fakes/fake_browser_state.cc>

#undef SetProtocolHandler
