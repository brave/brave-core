/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_

#include "chrome/browser/profiles/profile_observer.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/event_router.h"

#define OnOffTheRecordProfileCreated(...)                 \
  OnOffTheRecordProfileCreated_ChromiumImpl(__VA_ARGS__); \
  void OnOffTheRecordProfileCreated(__VA_ARGS__)

#define MaybeStartListening(...)    \
  MaybeStartListening(__VA_ARGS__); \
  friend struct OnCookieChangeExposeForTesting

#define GetFactoryInstance(...)    \
  GetFactoryInstance(__VA_ARGS__); \
  friend struct OnCookieChangeExposeForTesting

#include "src/chrome/browser/extensions/api/cookies/cookies_api.h"  // IWYU pragma: export

namespace extensions {
struct OnCookieChangeExposeForTesting {
  static void CallOnCookieChangeForOtr(CookiesAPI* cookies_api);
};
}  // namespace extensions

#undef OnOffTheRecordProfileCreated
#undef MaybeStartListening
#undef GetFactoryInstance

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_
