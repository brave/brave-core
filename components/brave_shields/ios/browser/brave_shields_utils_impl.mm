// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/ios/browser/brave_shields_utils_impl.h"

#include <Foundation/Foundation.h>

#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "ios/web/public/thread/web_thread.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation BraveShieldsUtilsImpl {
  raw_ptr<HostContentSettingsMap> _map;
  raw_ptr<PrefService> _localState;
  raw_ptr<PrefService> _profilePrefs;
}

- (instancetype)initWithHostContentSettingsMap:(HostContentSettingsMap*)map
                                    localState:(PrefService*)localState
                                  profilePrefs:(PrefService*)profilePrefs {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    _map = map;
    _localState = localState;
    _profilePrefs = profilePrefs;
  }
  return self;
}

- (bool)braveShieldsEnabledFor:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return brave_shields::GetBraveShieldsEnabled(_map, gurl);
}

- (void)setBraveShieldsEnabled:(bool)isEnabled forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  brave_shields::SetBraveShieldsEnabled(_map, isEnabled, gurl, _localState);
}

- (BraveShieldsAdBlockMode)defaultAdBlockMode {
  return [self adBlockModeForGURL:GURL()];
}

- (BraveShieldsAdBlockMode)adBlockModeForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return [self adBlockModeForGURL:gurl];
}

- (BraveShieldsAdBlockMode)adBlockModeForGURL:(GURL)gurl {
  return static_cast<BraveShieldsAdBlockMode>(
      brave_shields::GetAdBlockMode(_map, gurl));
}

- (void)setDefaultAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode {
  [self setAdBlockMode:adBlockMode forGURL:GURL()];
}

- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  [self setAdBlockMode:adBlockMode forGURL:gurl];
}

- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode forGURL:(GURL)gurl {
  brave_shields::SetAdBlockMode(
      _map, static_cast<brave_shields::mojom::AdBlockMode>(adBlockMode), gurl,
      _localState, _profilePrefs);
}

- (bool)isBlockScriptsEnabledByDefault {
  return [self blockScriptsEnabledForGURL:GURL()];
}

- (bool)blockScriptsEnabledForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return [self blockScriptsEnabledForGURL:gurl];
}

- (bool)blockScriptsEnabledForGURL:(GURL)gurl {
  return brave_shields::GetNoScriptEnabled(_map, gurl);
}

- (void)setBlockScriptsEnabledByDefault:(bool)isEnabled {
  [self setBlockScriptsEnabled:isEnabled forGURL:GURL()];
}

- (void)setBlockScriptsEnabled:(bool)isEnabled forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  [self setBlockScriptsEnabled:isEnabled forGURL:gurl];
}

- (void)setBlockScriptsEnabled:(bool)isEnabled forGURL:(GURL)gurl {
  brave_shields::SetIsNoScriptEnabled(_map, isEnabled, gurl, _localState);
}

- (bool)isBlockFingerprintingEnabledByDefault {
  return [self blockFingerprintingEnabledForGURL:GURL()];
}

- (bool)blockFingerprintingEnabledForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return [self blockFingerprintingEnabledForGURL:gurl];
}

- (bool)blockFingerprintingEnabledForGURL:(GURL)gurl {
  auto fingerprintMode = static_cast<BraveShieldsFingerprintMode>(
      brave_shields::GetFingerprintMode(_map, gurl));
  return fingerprintMode != BraveShieldsFingerprintModeAllowMode;
}

- (void)setBlockFingerprintingEnabledByDefault:(bool)isEnabled {
  [self setBlockFingerprintingEnabled:isEnabled forGURL:GURL()];
}

- (void)setBlockFingerprintingEnabled:(bool)isEnabled forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  [self setBlockFingerprintingEnabled:isEnabled forGURL:gurl];
}

- (void)setBlockFingerprintingEnabled:(bool)isEnabled forGURL:(GURL)gurl {
  // iOS only supports standard & allow FingerprintMode's.
  brave_shields::mojom::FingerprintMode fingerprintMode =
      isEnabled ? brave_shields::mojom::FingerprintMode::STANDARD_MODE
                : brave_shields::mojom::FingerprintMode::ALLOW_MODE;
  brave_shields::SetFingerprintMode(_map, fingerprintMode, gurl, _localState,
                                    _profilePrefs);
}

@end
