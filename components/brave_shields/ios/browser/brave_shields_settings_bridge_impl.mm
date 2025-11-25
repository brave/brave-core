// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/ios/browser/brave_shields_settings_bridge_impl.h"

#include <Foundation/Foundation.h>

#include <memory>

#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation BraveShieldsSettingsBridgeImpl {
  raw_ptr<brave_shields::BraveShieldsSettingsService> _braveShieldsSettings;
}

- (instancetype)initWithBraveShieldsSettings:
    (raw_ptr<brave_shields::BraveShieldsSettingsService>)braveShieldsSettings {
  if ((self = [super init])) {
    _braveShieldsSettings = braveShieldsSettings;
  }
  return self;
}

- (bool)braveShieldsEnabledFor:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return _braveShieldsSettings->GetBraveShieldsEnabled(gurl);
}

- (void)setBraveShieldsEnabled:(bool)isEnabled forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  _braveShieldsSettings->SetBraveShieldsEnabled(isEnabled, gurl);
}

- (BraveShieldsAdBlockMode)defaultAdBlockMode {
  return static_cast<BraveShieldsAdBlockMode>(
      _braveShieldsSettings->GetDefaultAdBlockMode());
}

- (BraveShieldsAdBlockMode)adBlockModeForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return static_cast<BraveShieldsAdBlockMode>(
      _braveShieldsSettings->GetAdBlockMode(gurl));
}

- (void)setDefaultAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode {
  _braveShieldsSettings->SetDefaultAdBlockMode(
      static_cast<brave_shields::mojom::AdBlockMode>(adBlockMode));
}

- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  _braveShieldsSettings->SetAdBlockMode(
      static_cast<brave_shields::mojom::AdBlockMode>(adBlockMode), gurl);
}

- (bool)isBlockScriptsEnabledByDefault {
  return _braveShieldsSettings->IsNoScriptEnabledByDefault();
}

- (bool)blockScriptsEnabledForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return _braveShieldsSettings->IsNoScriptEnabled(gurl);
}

- (void)setBlockScriptsEnabledByDefault:(bool)isEnabled {
  _braveShieldsSettings->SetNoScriptEnabledByDefault(isEnabled);
}

- (void)setBlockScriptsEnabled:(bool)isEnabled forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  _braveShieldsSettings->SetNoScriptEnabled(isEnabled, gurl);
}

- (BraveShieldsFingerprintMode)defaultFingerprintMode {
  return static_cast<BraveShieldsFingerprintMode>(
      _braveShieldsSettings->GetDefaultFingerprintMode());
}

- (BraveShieldsFingerprintMode)fingerprintModeForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return static_cast<BraveShieldsFingerprintMode>(
      _braveShieldsSettings->GetFingerprintMode(gurl));
}

- (void)setDefaultFingerprintMode:(BraveShieldsFingerprintMode)fingerprintMode {
  _braveShieldsSettings->SetDefaultFingerprintMode(
      static_cast<brave_shields::mojom::FingerprintMode>(fingerprintMode));
}

- (void)setFingerprintMode:(BraveShieldsFingerprintMode)fingerprintMode
                    forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  _braveShieldsSettings->SetFingerprintMode(
      static_cast<brave_shields::mojom::FingerprintMode>(fingerprintMode),
      gurl);
}

- (BraveShieldsAutoShredMode)defaultAutoShredMode {
  return static_cast<BraveShieldsAutoShredMode>(
      _braveShieldsSettings->GetDefaultAutoShredMode());
}

- (BraveShieldsAutoShredMode)autoShredModeForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return static_cast<BraveShieldsAutoShredMode>(
      _braveShieldsSettings->GetAutoShredMode(gurl));
}

- (void)setDefaultAutoShredMode:(BraveShieldsAutoShredMode)autoShredMode {
  _braveShieldsSettings->SetDefaultAutoShredMode(
      static_cast<brave_shields::mojom::AutoShredMode>(autoShredMode));
}

- (void)setAutoShredMode:(BraveShieldsAutoShredMode)autoShredMode
                  forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  _braveShieldsSettings->SetAutoShredMode(
      static_cast<brave_shields::mojom::AutoShredMode>(autoShredMode), gurl);
}

- (BOOL)isShieldsDisabledOnAnyHostMatchingDomainOf:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return _braveShieldsSettings->IsShieldsDisabledOnAnyHostMatchingDomainOf(
      gurl);
}

@end
