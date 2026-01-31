/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_UI_BRAVE_L10N_UTIL_BRIDGE_H_
#define BRAVE_IOS_BROWSER_UI_BRAVE_L10N_UTIL_BRIDGE_H_

#import <Foundation/Foundation.h>

#ifdef __cplusplus
#import "ui/base/l10n/l10n_util_mac_bridge.h"
#else
#import "l10n_util_mac_bridge.h"
#endif

OBJC_VISIBLE
@interface BraveL10nUtils : L10nUtils
@end

#endif  // BRAVE_IOS_BROWSER_UI_BRAVE_L10N_UTIL_BRIDGE_H_
