// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_UTILS_IOS_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_UTILS_IOS_PRIVATE_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/api/brave_shields/brave_shields_utils_ios.h"

NS_ASSUME_NONNULL_BEGIN

class ProfileIOS;

@interface BraveShieldsUtilsIOS (Private)
- (instancetype)initWithBrowserState:(ProfileIOS*)profile;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_UTILS_IOS_PRIVATE_H_
