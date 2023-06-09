/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_APP_CONTROLLER_MAC_H_
#define BRAVE_BROWSER_BRAVE_APP_CONTROLLER_MAC_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/app_controller_mac.h"

// Manages logic to switch hotkey between copy and copy clean link item.
@interface BraveAppController : AppController

@property(readonly, nonatomic, class) BraveAppController* sharedController;

@property(nonatomic, assign) NSMenuItem* copyMenuItem;
@property(nonatomic, assign) NSMenuItem* copyCleanLinkMenuItem;

// NOLINTNEXTLINE
- (NSMenuItem*)copyMenuItem __attribute__((objc_method_family(none)));
// NOLINTNEXTLINE
- (NSMenuItem*)copyCleanLinkMenuItem __attribute__((objc_method_family(none)));

- (instancetype)init;
@end

#endif  // BRAVE_BROWSER_BRAVE_APP_CONTROLLER_MAC_H_
