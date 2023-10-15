/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_IMPL_H_

#include "chrome/browser/profiles/profile.h"

#define ShouldRestoreOldSessionCookies(...)                 \
  ShouldRestoreOldSessionCookies_ChromiumImpl(__VA_ARGS__); \
  bool ShouldRestoreOldSessionCookies(__VA_ARGS__)

#include "src/chrome/browser/profiles/profile_impl.h"  // IWYU pragma: export

#undef ShouldRestoreOldSessionCookies

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_IMPL_H_
