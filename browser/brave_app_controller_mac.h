/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_APP_CONTROLLER_MAC_H_
#define BRAVE_BROWSER_BRAVE_APP_CONTROLLER_MAC_H_

#import "chrome/browser/app_controller_mac.h"

// Manages logic to switch hotkey between copy and copy clean link item.
@interface BraveAppController : AppController {
  NSMenuItem* _copyMenuItem;
  NSMenuItem* _copyCleanLinkMenuItem;
}

@end

#endif  // BRAVE_BROWSER_BRAVE_APP_CONTROLLER_MAC_H_
