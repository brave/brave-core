/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SCREEN_AI_SCREEN_AI_INSTALL_STATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SCREEN_AI_SCREEN_AI_INSTALL_STATE_H_

#define ShouldInstall                                   \
  ShouldInstall_ChromiumImpl(PrefService* local_state); \
  static bool ShouldInstall

#include "src/chrome/browser/screen_ai/screen_ai_install_state.h"  // IWYU pragma: export

#undef ShouldInstall

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SCREEN_AI_SCREEN_AI_INSTALL_STATE_H_
