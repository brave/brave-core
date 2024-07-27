/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_ACTIONS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_ACTIONS_H_

#define InitializeBrowserActions    \
  UnUsed() {}                       \
  friend class BraveBrowserActions; \
  virtual void InitializeBrowserActions

#include "src/chrome/browser/ui/browser_actions.h"  // IWYU pragma: export

#undef InitializeBrowserActions

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_ACTIONS_H_
