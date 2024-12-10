// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_PREFS_BROWSER_PREFS_H_
#define BRAVE_IOS_BROWSER_API_PREFS_BROWSER_PREFS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface BrowserPrefs : NSObject
@property(nonatomic) BOOL httpsUpgradesEnabled;
@property(nonatomic) BOOL httpsOnlyModeEnabled;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_PREFS_BROWSER_PREFS_H_
