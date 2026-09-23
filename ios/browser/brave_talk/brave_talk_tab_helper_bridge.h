// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_HELPER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_HELPER_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol BraveTalkTabHelperBridge
@required

- (void)launchBraveTalkWithRoom:(NSString*)room jwtKey:(NSString*)jwtKey;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_HELPER_BRIDGE_H_
