/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_

#include "chrome/browser/profiles/profile_observer.h"

#define OnOffTheRecordProfileCreated(...)                 \
  OnOffTheRecordProfileCreated_ChromiumImpl(__VA_ARGS__); \
  void OnOffTheRecordProfileCreated(__VA_ARGS__)

#include "src/chrome/browser/extensions/api/cookies/cookies_api.h"  // IWYU pragma: export

#undef OnOffTheRecordProfileCreated

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_EXTENSIONS_API_COOKIES_COOKIES_API_H_
