/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_EXCLUSIVE_ACCESS_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_EXCLUSIVE_ACCESS_CONTEXT_H_

#include "base/functional/callback.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_bubble_hide_callback.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_bubble_type.h"

#define UpdateExclusiveAccessBubble(...)                   \
  UpdateExclusiveAccessBubble_ChromiumImpl(__VA_ARGS__) {} \
  virtual void UpdateExclusiveAccessBubble(__VA_ARGS__)

#include <chrome/browser/ui/exclusive_access/exclusive_access_context.h>  // IWYU pragma: export
#undef UpdateExclusiveAccessBubble

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_EXCLUSIVE_ACCESS_CONTEXT_H_
