/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_COCOA_KEYSTONE_INFOBAR_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_COCOA_KEYSTONE_INFOBAR_DELEGATE_H_

#define PromotionInfoBar                     \
  PromotionInfoBar_Unused(Profile* profile); \
  static void PromotionInfoBar

#include "src/chrome/browser/ui/cocoa/keystone_infobar_delegate.h"  // IWYU pragma: export

#undef PromotionInfoBar

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_COCOA_KEYSTONE_INFOBAR_DELEGATE_H_
