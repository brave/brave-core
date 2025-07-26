// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_UTILS_IOS_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_UTILS_IOS_H_

#import <Foundation/Foundation.h>

#import "brave_shields_panel.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveShieldsUtilsIOS : NSObject

- (instancetype)init NS_UNAVAILABLE;

- (BOOL)braveShieldsEnabledFor:(NSURL*)url
                     isPrivate:(BOOL)isPrivate
    NS_SWIFT_NAME(isBraveShieldsEnabled(for:isPrivate:));
- (void)setBraveShieldsEnabled:(BOOL)isEnabled
                        forURL:(NSURL*)url
                     isPrivate:(BOOL)isPrivate;

@property(nonatomic) BraveShieldsAdBlockMode defaultAdBlockMode;
- (BraveShieldsAdBlockMode)adBlockModeForURL:(NSURL*)url
                                   isPrivate:(BOOL)isPrivate
    NS_SWIFT_NAME(adBlockMode(for:isPrivate:));
- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode
                forURL:(NSURL*)url
             isPrivate:(BOOL)isPrivate;

@property(nonatomic, getter=isBlockScriptsEnabledByDefault)
    BOOL blockScriptsEnabledByDefault;
- (BOOL)blockScriptsEnabledForURL:(NSURL*)url
                        isPrivate:(BOOL)isPrivate
    NS_SWIFT_NAME(isBlockScriptsEnabled(for:isPrivate:));
- (void)setBlockScriptsEnabled:(BOOL)isEnabled
                        forURL:(NSURL*)url
                     isPrivate:(BOOL)isPrivate;

@property(nonatomic, getter=isBlockFingerprintingEnabledByDefault)
    BOOL blockFingerprintingEnabledByDefault;
- (BOOL)blockFingerprintingEnabledForURL:(NSURL*)url
                               isPrivate:(BOOL)isPrivate
    NS_SWIFT_NAME(isBlockFingerprintingEnabled(for:isPrivate:));
- (void)setBlockFingerprintingEnabled:(BOOL)isEnabled
                               forURL:(NSURL*)url
                            isPrivate:(BOOL)isPrivate;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_UTILS_IOS_H_
