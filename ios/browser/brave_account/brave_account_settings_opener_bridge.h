/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_BRIDGE_H_

#import <Foundation/Foundation.h>

#if __swift__
#import "brave_account.mojom.objc.h"
#else
#include "brave/components/brave_account/mojom/brave_account.mojom.objc.h"
#endif

NS_ASSUME_NONNULL_BEGIN

/// A bridge for opening a settings page in place of the WebUI page that only
/// links to it.
OBJC_EXPORT
@protocol BraveAccountSettingsOpenerBridge
- (void)openSettingsPage:(BraveAccountSettingsPage)page
    NS_SWIFT_NAME(openSettingsPage(page:));
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_BRIDGE_H_
