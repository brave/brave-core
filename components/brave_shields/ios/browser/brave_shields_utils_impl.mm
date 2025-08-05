// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/ios/browser/brave_shields_utils_impl.h"

#include <Foundation/Foundation.h>

#include "brave/components/brave_shields/core/browser/brave_shields_settings.h"
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
  return brave_shields::GetIsBraveShieldsEnabled(_map, gurl);
}

- (void)setBraveShieldsEnabled:(bool)isEnabled forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  brave_shields::SetIsBraveShieldsEnabled(_map, isEnabled, gurl, _localState);
}

- (BraveShieldsAdBlockMode)defaultAdBlockMode {
  return static_cast<BraveShieldsAdBlockMode>(
      brave_shields::GetDefaultAdBlockMode(_map));
}

- (BraveShieldsAdBlockMode)adBlockModeForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return static_cast<BraveShieldsAdBlockMode>(
      brave_shields::GetAdBlockMode(_map, gurl));
}

- (void)setDefaultAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode {
  brave_shields::SetDefaultAdBlockMode(
      _map, static_cast<brave_shields::mojom::AdBlockMode>(adBlockMode),
      _localState, _profilePrefs);
}

- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  brave_shields::SetAdBlockMode(
      _map, static_cast<brave_shields::mojom::AdBlockMode>(adBlockMode), gurl,
      _localState, _profilePrefs);
}

- (bool)isBlockScriptsEnabledByDefault {
  return brave_shields::GetNoScriptEnabledByDefault(_map);
}

- (bool)blockScriptsEnabledForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return brave_shields::GetNoScriptEnabled(_map, gurl);
}

- (void)setBlockScriptsEnabledByDefault:(bool)isEnabled {
  brave_shields::SetIsNoScriptEnabledByDefault(_map, isEnabled, _localState);
}

- (void)setBlockScriptsEnabled:(bool)isEnabled forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  brave_shields::SetIsNoScriptEnabled(_map, isEnabled, gurl, _localState);
}

- (BraveShieldsFingerprintMode)defaultFingerprintMode {
  return static_cast<BraveShieldsFingerprintMode>(
      brave_shields::GetDefaultFingerprintMode(_map));
}

- (BraveShieldsFingerprintMode)fingerprintModeForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return static_cast<BraveShieldsFingerprintMode>(
      brave_shields::GetFingerprintMode(_map, gurl));
}

- (void)setDefaultFingerprintMode:(BraveShieldsFingerprintMode)fingerprintMode {
  brave_shields::SetDefaultFingerprintMode(
      _map, static_cast<brave_shields::mojom::FingerprintMode>(fingerprintMode),
      _localState, _profilePrefs);
}

- (void)setFingerprintMode:(BraveShieldsFingerprintMode)fingerprintMode
                    forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  brave_shields::SetFingerprintMode(
      _map, static_cast<brave_shields::mojom::FingerprintMode>(fingerprintMode),
      gurl, _localState, _profilePrefs);
}

@end
