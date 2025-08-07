// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_SETTINGS_BRIDGE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_SETTINGS_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "brave/components/brave_shields/ios/browser/brave_shields_settings_bridge.h"
#include "brave/components/brave_shields/ios/common/shields_settings.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

class HostContentSettingsMap;
class PrefService;

@interface BraveShieldsSettingsBridgeImpl
    : NSObject <BraveShieldsSettingsBridge>

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithHostContentSettingsMap:(HostContentSettingsMap*)map
                                    localState:(PrefService*)localState
                                  profilePrefs:(PrefService*)profilePrefs;

- (BOOL)braveShieldsEnabledFor:(NSURL*)url;
- (void)setBraveShieldsEnabled:(BOOL)isEnabled forURL:(NSURL*)url;

@property(nonatomic) BraveShieldsAdBlockMode defaultAdBlockMode;
- (BraveShieldsAdBlockMode)adBlockModeForURL:(NSURL*)url;
- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode forURL:(NSURL*)url;

@property(nonatomic, getter=isBlockScriptsEnabledByDefault)
    BOOL blockScriptsEnabledByDefault;
- (BOOL)blockScriptsEnabledForURL:(NSURL*)url;
- (void)setBlockScriptsEnabled:(BOOL)isEnabled forURL:(NSURL*)url;

@property(nonatomic) BraveShieldsFingerprintMode defaultFingerprintMode;
- (BraveShieldsFingerprintMode)fingerprintModeForURL:(NSURL*)url;
- (void)setFingerprintMode:(BraveShieldsFingerprintMode)fingerprintMode
                    forURL:(NSURL*)url;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_SETTINGS_BRIDGE_IMPL_H_
