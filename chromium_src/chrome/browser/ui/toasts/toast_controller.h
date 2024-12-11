/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TOASTS_TOAST_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TOASTS_TOAST_CONTROLLER_H_

#define MaybeShowToast                             \
  MaybeShowToast_ChromiumImpl(ToastParams params); \
  bool MaybeShowToast

#include "src/chrome/browser/ui/toasts/toast_controller.h"  // IWYU pragma: export

#undef MaybeShowToast

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TOASTS_TOAST_CONTROLLER_H_
