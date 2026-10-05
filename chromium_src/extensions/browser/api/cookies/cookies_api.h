/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_API_COOKIES_COOKIES_API_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_API_COOKIES_COOKIES_API_H_

#include <extensions/browser/api/cookies/cookies_api.h>  // IWYU pragma: export

#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/event_router.h"

namespace extensions {
struct OnCookieChangeExposeForTesting {
  static void CallOnCookieChangeForOtr(CookiesAPI* cookies_api);
};
}  // namespace extensions

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_API_COOKIES_COOKIES_API_H_
