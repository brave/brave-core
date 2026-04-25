/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_UI_L10N_UTILS_TYPED_BRIDGE_H_
#define BRAVE_IOS_BROWSER_UI_L10N_UTILS_TYPED_BRIDGE_H_

#import <Foundation/Foundation.h>

#ifdef __cplusplus
#import "ui/base/l10n/l10n_util_mac_bridge.h"
#else
#import "l10n_util_mac_bridge.h"
#endif

typedef MessageID MessageIDTyped NS_TYPED_ENUM;

NS_ASSUME_NONNULL_BEGIN

@interface L10nUtils (Typed)

+ (NSString*)stringForMessageIDTyped:(MessageIDTyped)messageID
    NS_SWIFT_NAME(string(messageId:));

+ (NSString*)stringWithFixupForMessageIDTyped:(MessageIDTyped)messageID
    NS_SWIFT_NAME(stringWithFixup(messageId:));

+ (NSString*)formatStringForMessageIDTyped:(MessageIDTyped)messageID
                                  argument:(NSString*)argument
    NS_SWIFT_NAME(formatString(messageId:argument:));

+ (NSString*)pluralStringForMessageIDTyped:(MessageIDTyped)messageID
                                    number:(NSInteger)number
    NS_SWIFT_NAME(pluralString(messageId:number:));

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_UI_L10N_UTILS_TYPED_BRIDGE_H_
