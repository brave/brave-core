/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_HEADLESS_LIB_BROWSER_HEADLESS_CLIENT_HINTS_CONTROLLER_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_HEADLESS_LIB_BROWSER_HEADLESS_CLIENT_HINTS_CONTROLLER_DELEGATE_H_

#include "content/public/browser/client_hints_controller_delegate.h"
#include "headless/public/headless_browser.h"

#define GetUserAgentMetadata()                             \
  BraveGetUserAgentMetadata(bool showBraveBrand) override; \
  blink::UserAgentMetadata GetUserAgentMetadata()

#include "src/headless/lib/browser/headless_client_hints_controller_delegate.h"  // IWYU pragma: export

#undef GetUserAgentMetadata

#endif  // BRAVE_CHROMIUM_SRC_HEADLESS_LIB_BROWSER_HEADLESS_CLIENT_HINTS_CONTROLLER_DELEGATE_H_
