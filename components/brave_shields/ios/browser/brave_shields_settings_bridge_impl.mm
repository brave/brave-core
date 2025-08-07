// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/ios/browser/brave_shields_settings_bridge_impl.h"

#include <Foundation/Foundation.h>

#include "brave/components/brave_shields/core/browser/brave_shields_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "ios/web/public/thread/web_thread.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation BraveShieldsSettingsBridgeImpl {
  std::unique_ptr<brave_shields::BraveShieldsSettings> _brave_shields_settings;
}

- (instancetype)initWithHostContentSettingsMap:(HostContentSettingsMap*)map
                                    localState:(PrefService*)localState
                                  profilePrefs:(PrefService*)profilePrefs {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    _brave_shields_settings =
        std::make_unique<brave_shields::BraveShieldsSettings>(map, localState,
                                                              profilePrefs);
  }
  return self;
}

- (bool)braveShieldsEnabledFor:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return _brave_shields_settings->GetBraveShieldsEnabled(gurl);
}

- (void)setBraveShieldsEnabled:(bool)isEnabled forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  _brave_shields_settings->SetBraveShieldsEnabled(isEnabled, gurl);
}

- (BraveShieldsAdBlockMode)defaultAdBlockMode {
  return static_cast<BraveShieldsAdBlockMode>(
      _brave_shields_settings->GetDefaultAdBlockMode());
}

- (BraveShieldsAdBlockMode)adBlockModeForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return static_cast<BraveShieldsAdBlockMode>(
      _brave_shields_settings->GetAdBlockMode(gurl));
}

- (void)setDefaultAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode {
  _brave_shields_settings->SetDefaultAdBlockMode(
      static_cast<brave_shields::mojom::AdBlockMode>(adBlockMode));
}

- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  _brave_shields_settings->SetAdBlockMode(
      static_cast<brave_shields::mojom::AdBlockMode>(adBlockMode), gurl);
}

- (bool)isBlockScriptsEnabledByDefault {
  return _brave_shields_settings->GetNoScriptEnabledByDefault();
}

- (bool)blockScriptsEnabledForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return _brave_shields_settings->GetNoScriptEnabled(gurl);
}

- (void)setBlockScriptsEnabledByDefault:(bool)isEnabled {
  _brave_shields_settings->SetIsNoScriptEnabledByDefault(isEnabled);
}

- (void)setBlockScriptsEnabled:(bool)isEnabled forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  _brave_shields_settings->SetIsNoScriptEnabled(isEnabled, gurl);
}

- (BraveShieldsFingerprintMode)defaultFingerprintMode {
  return static_cast<BraveShieldsFingerprintMode>(
      _brave_shields_settings->GetDefaultFingerprintMode());
}

- (BraveShieldsFingerprintMode)fingerprintModeForURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  return static_cast<BraveShieldsFingerprintMode>(
      _brave_shields_settings->GetFingerprintMode(gurl));
}

- (void)setDefaultFingerprintMode:(BraveShieldsFingerprintMode)fingerprintMode {
  _brave_shields_settings->SetDefaultFingerprintMode(
      static_cast<brave_shields::mojom::FingerprintMode>(fingerprintMode));
}

- (void)setFingerprintMode:(BraveShieldsFingerprintMode)fingerprintMode
                    forURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  _brave_shields_settings->SetFingerprintMode(
      static_cast<brave_shields::mojom::FingerprintMode>(fingerprintMode),
      gurl);
}

@end
