/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_APP_CONTROLLER_MAC_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_APP_CONTROLLER_MAC_H_

#if defined(__OBJC__)

#define mainMenuCreated   \
  dummy;                  \
  -(void)mainMenuCreated; \
  -(instancetype)initForBrave
#endif  // defined(__OBJC__)

#import "src/chrome/browser/app_controller_mac.h"

#if defined(__OBJC__)
#undef mainMenuCreated
#endif  // defined(__OBJC__)

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_APP_CONTROLLER_MAC_H_
