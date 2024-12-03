/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "chrome/browser/app_controller_mac.h"

#import "brave/browser/brave_app_controller_mac.h"

// Work around a bug in macOS 14. The application name shown in the menu bar
// should be the CFBundleName, but a bug in the menu rewrite causes the
// CFBundleDisplayName to be swapped in. What's happening here is that the
// NSMenuBarDisplayManager registers for a notification that the app name
// changed, and then, when the notification comes in, it calls a Process
// Manager API that returns the CFBundleDisplayName which is not appropriate
// for this use. Work around this by unregistering the NSMenuBarDisplayManager
// for the notification. See https://crbug.com/1487224 and FB13192263.
#define BRAVE_APP_CONTROLLER_MAC_MAIN_MENU_CREATED                          \
  if (id sharedMenuBarDisplayManager = [NSClassFromString(                  \
          @"NSMenuBarDisplayManager") performSelector:@selector(shared)]) { \
    [NSWorkspace.sharedWorkspace.notificationCenter                         \
        removeObserver:sharedMenuBarDisplayManager                          \
                  name:@"NSWorkspaceApplicationNameDidChangeNotification"   \
                object:nil];                                                \
  }

#include "src/chrome/browser/app_controller_mac.mm"
#undef BRAVE_APP_CONTROLLER_MAC_MAIN_MENU_CREATED
