// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_SETTINGS_BRIDGE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_SETTINGS_BRIDGE_H_

#ifdef __cplusplus
#include "brave/components/brave_shields/ios/common/shields_settings.mojom.objc.h"
#else
#import "shields_settings.mojom.objc.h"
#endif

NS_SWIFT_NAME(BraveShieldsSettings)
@protocol BraveShieldsSettingsBridge

- (BOOL)braveShieldsEnabledFor:(NSURL*)url
    NS_SWIFT_NAME(isBraveShieldsEnabled(for:));
- (void)setBraveShieldsEnabled:(BOOL)isEnabled forURL:(NSURL*)url;

@property(nonatomic) BraveShieldsAdBlockMode defaultAdBlockMode;
- (BraveShieldsAdBlockMode)adBlockModeForURL:(NSURL*)url
    NS_SWIFT_NAME(adBlockMode(for:));
- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode forURL:(NSURL*)url;

@property(nonatomic, getter=isBlockScriptsEnabledByDefault)
    BOOL blockScriptsEnabledByDefault;
- (BOOL)blockScriptsEnabledForURL:(NSURL*)url
    NS_SWIFT_NAME(isBlockScriptsEnabled(for:));
- (void)setBlockScriptsEnabled:(BOOL)isEnabled forURL:(NSURL*)url;

@property(nonatomic) BraveShieldsFingerprintMode defaultFingerprintMode;
- (BraveShieldsFingerprintMode)fingerprintModeForURL:(NSURL*)url
    NS_SWIFT_NAME(fingerprintMode(for:));
- (void)setFingerprintMode:(BraveShieldsFingerprintMode)fingerprintMode
                    forURL:(NSURL*)url;

@property(nonatomic) BraveShieldsAutoShredMode defaultAutoShredMode;
- (BraveShieldsAutoShredMode)autoShredModeForURL:(NSURL*)url
    NS_SWIFT_NAME(autoShredMode(for:));
- (void)setAutoShredMode:(BraveShieldsAutoShredMode)autoShredMode
                  forURL:(NSURL*)url;

- (NSArray<NSURL*>*)domainsWithAutoShredMode:
    (BraveShieldsAutoShredMode)autoShredMode NS_SWIFT_NAME(domains(with:));
@end

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_IOS_BROWSER_BRAVE_SHIELDS_SETTINGS_BRIDGE_H_
