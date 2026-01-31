/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_UI_BRAVE_L10N_UTIL_BRIDGE_H_
#define BRAVE_IOS_BROWSER_UI_BRAVE_L10N_UTIL_BRIDGE_H_

#import <Foundation/Foundation.h>

typedef int BraveMessageID NS_TYPED_ENUM;

NS_ASSUME_NONNULL_BEGIN

OBJC_VISIBLE
@interface BraveL10nUtils : NSObject

+ (NSString*)stringForBraveMessageID:(BraveMessageID)messageID
    NS_SWIFT_NAME(string(messageId:));

+ (NSString*)stringWithFixupForBraveMessageID:(BraveMessageID)messageID
    NS_SWIFT_NAME(stringWithFixup(messageId:));

+ (NSString*)formatStringForBraveMessageID:(BraveMessageID)messageID
                                  argument:(NSString*)argument
    NS_SWIFT_NAME(formatString(messageId:argument:));

+ (NSString*)pluralStringForBraveMessageID:(BraveMessageID)messageID
                                    number:(NSInteger)number
    NS_SWIFT_NAME(pluralString(messageId:number:));

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_UI_BRAVE_L10N_UTIL_BRIDGE_H_
