// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_UTILS_IOS_PRIVATE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_UTILS_IOS_PRIVATE_H_

#import <Foundation/Foundation.h>

#include "brave/components/brave_shields/ios/browser/brave_shields_utils_ios.h"

NS_ASSUME_NONNULL_BEGIN

class HostContentSettingsMap;
class PrefService;

@interface BraveShieldsUtilsIOS (Private)
- (instancetype)initWithHostContentSettingsMap:(HostContentSettingsMap*)map
                                    localState:(PrefService*)localState
                                  profilePrefs:(PrefService*)profilePrefs;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_UTILS_IOS_PRIVATE_H_
