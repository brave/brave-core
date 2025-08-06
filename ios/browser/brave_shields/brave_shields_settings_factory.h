// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_FACTORY_H_

#import <Foundation/Foundation.h>

@protocol BraveShieldsSettings;
@protocol ProfileBridge;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveShieldsSettingsFactory : NSObject
+ (id<BraveShieldsSettings>)getForProfile:(id<ProfileBridge>)profile
    NS_SWIFT_NAME(get(profile:));
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_BRAVE_SHIELDS_SETTINGS_FACTORY_H_
