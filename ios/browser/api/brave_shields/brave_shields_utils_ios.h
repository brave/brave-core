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

- (bool)getBraveShieldsEnabledFor:(NSURL*)url
                        isPrivate:(bool)isPrivate
    NS_SWIFT_NAME(isBraveShieldsEnabled(for:isPrivate:));
- (void)setBraveShieldsEnabled:(bool)isEnabled
                        forURL:(NSURL*)url
                     isPrivate:(bool)isPrivate;

- (BraveShieldsAdBlockMode)
    getDefaultAdBlockMode NS_SWIFT_NAME(defaultAdBlockMode());
- (BraveShieldsAdBlockMode)getAdBlockModeForURL:(NSURL*)url
                                      isPrivate:(bool)isPrivate
    NS_SWIFT_NAME(adBlockMode(for:isPrivate:));
- (void)setDefaultAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode;
- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode
                forURL:(NSURL*)url
             isPrivate:(bool)isPrivate;

- (bool)getBlockScriptsEnabledByDefault NS_SWIFT_NAME(isBlockScriptsEnabledByDefault());
- (bool)getBlockScriptsEnabledForURL:(NSURL*)url
                           isPrivate:(bool)isPrivate
    NS_SWIFT_NAME(isBlockScriptsEnabled(for:isPrivate:));
- (void)setBlockScriptsEnabledByDefault:(bool)isEnabled;
- (void)setBlockScriptsEnabled:(bool)isEnabled
                        forURL:(NSURL*)url
                     isPrivate:(bool)isPrivate;

- (bool)getBlockFingerprintingEnabledByDefault NS_SWIFT_NAME(isBlockFingerprintingEnabledByDefault());
- (bool)getBlockFingerprintingEnabledForURL:(NSURL*)url
                                  isPrivate:(bool)isPrivate
    NS_SWIFT_NAME(isBlockFingerprintingEnabled(for:isPrivate:));
- (void)setBlockFingerprintingEnabledByDefault:(bool)isEnabled;
- (void)setBlockFingerprintingEnabled:(bool)isEnabled
                               forURL:(NSURL*)url
                            isPrivate:(bool)isPrivate;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_UTILS_IOS_H_
