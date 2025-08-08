// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_UTILS_H_
#define BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_UTILS_H_

#import <Foundation/Foundation.h>

@protocol PrefServiceBridge;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface AIChatUtils : NSObject
/// Whether or not AI Chat/Leo should be accessible to the user within the UI.
/// When this returns false, ensure that all instances of Leo are removed from
/// the browser (such as settings, menu buttons, etc.)
+ (BOOL)isAIChatEnabledForPrefService:(id<PrefServiceBridge>)prefService
    NS_SWIFT_NAME(isAIChatEnabled(for:));
- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_UTILS_H_
