// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BRAVE_SHIELDS_UTILS_IOS_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BRAVE_SHIELDS_UTILS_IOS_H_

#import <Foundation/Foundation.h>

#include "brave/components/brave_shields/ios/browser/brave_shields_utils.h"
#import "brave_shields_panel.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

@interface BraveShieldsUtilsIOS : NSObject <BraveShieldsUtils>

- (instancetype)init NS_UNAVAILABLE;

- (BOOL)braveShieldsEnabledFor:(NSURL*)url isPrivate:(BOOL)isPrivate;
- (void)setBraveShieldsEnabled:(BOOL)isEnabled
                        forURL:(NSURL*)url
                     isPrivate:(BOOL)isPrivate;

@property(nonatomic) BraveShieldsAdBlockMode defaultAdBlockMode;
- (BraveShieldsAdBlockMode)adBlockModeForURL:(NSURL*)url
                                   isPrivate:(BOOL)isPrivate;
- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode
                forURL:(NSURL*)url
             isPrivate:(BOOL)isPrivate;

@property(nonatomic, getter=isBlockScriptsEnabledByDefault)
    BOOL blockScriptsEnabledByDefault;
- (BOOL)blockScriptsEnabledForURL:(NSURL*)url isPrivate:(BOOL)isPrivate;
- (void)setBlockScriptsEnabled:(BOOL)isEnabled
                        forURL:(NSURL*)url
                     isPrivate:(BOOL)isPrivate;

@property(nonatomic, getter=isBlockFingerprintingEnabledByDefault)
    BOOL blockFingerprintingEnabledByDefault;
- (BOOL)blockFingerprintingEnabledForURL:(NSURL*)url isPrivate:(BOOL)isPrivate;
- (void)setBlockFingerprintingEnabled:(BOOL)isEnabled
                               forURL:(NSURL*)url
                            isPrivate:(BOOL)isPrivate;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BRAVE_SHIELDS_UTILS_IOS_H_
