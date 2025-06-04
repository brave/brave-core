/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_CLIENT_HINTS_CLIENT_HINTS_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_CLIENT_HINTS_CLIENT_HINTS_H_

#define UpdateNavigationRequestClientUaHeaders(...)                 \
  UpdateNavigationRequestClientUaHeaders_ChromiumImpl(__VA_ARGS__); \
  CONTENT_EXPORT void UpdateNavigationRequestClientUaHeaders(__VA_ARGS__)

#include "src/content/browser/client_hints/client_hints.h"  // IWYU pragma: export

#undef UpdateNavigationRequestClientUaHeaders

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_CLIENT_HINTS_CLIENT_HINTS_H_
