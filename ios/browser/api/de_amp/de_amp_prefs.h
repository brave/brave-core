// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_DE_AMP_DE_AMP_PREFS_H_
#define BRAVE_IOS_BROWSER_API_DE_AMP_DE_AMP_PREFS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface DeAmpPrefs : NSObject
- (instancetype)init NS_UNAVAILABLE;

/**
 True or false if de-amp is enabled
 */
@property(nonatomic) bool isDeAmpEnabled;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_DE_AMP_DE_AMP_PREFS_H_
