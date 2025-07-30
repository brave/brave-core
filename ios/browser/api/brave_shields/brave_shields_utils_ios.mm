// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/brave_shields/brave_shields_utils_ios.h"

#include <Foundation/Foundation.h>

#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/thread/web_thread.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

@implementation BraveShieldsUtilsIOS {
  raw_ptr<ProfileIOS> _profile;  // NOT OWNED
}

- (instancetype)initWithBrowserState:(ProfileIOS*)profile {
  if ((self = [super init])) {
    DCHECK_CURRENTLY_ON(web::WebThread::UI);
    _profile = profile;
  }
  return self;
}

- (HostContentSettingsMap*)hostContentSettingsMapForPrivate:(bool)isPrivate {
  auto* profile =
      isPrivate ? _profile->GetOffTheRecordProfile() : _profile.get();
  return ios::HostContentSettingsMapFactory::GetForProfile(profile);
}

- (bool)braveShieldsEnabledFor:(NSURL*)url isPrivate:(bool)isPrivate {
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  GURL gurl = net::GURLWithNSURL(url);
  return brave_shields::GetBraveShieldsEnabled(map, gurl);
}

- (void)setBraveShieldsEnabled:(bool)isEnabled
                        forURL:(NSURL*)url
                     isPrivate:(bool)isPrivate {
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  GURL gurl = net::GURLWithNSURL(url);
  auto* localState = GetApplicationContext()->GetLocalState();
  brave_shields::SetBraveShieldsEnabled(map, isEnabled, gurl, localState);
}

- (BraveShieldsAdBlockMode)defaultAdBlockMode {
  return [self adBlockModeForGURL:GURL() isPrivate:false];
}

- (BraveShieldsAdBlockMode)adBlockModeForURL:(NSURL*)url
                                   isPrivate:(bool)isPrivate {
  GURL gurl = net::GURLWithNSURL(url);
  return [self adBlockModeForGURL:gurl isPrivate:isPrivate];
}

- (BraveShieldsAdBlockMode)adBlockModeForGURL:(GURL)gurl
                                    isPrivate:(bool)isPrivate {
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  return static_cast<BraveShieldsAdBlockMode>(
      brave_shields::GetAdBlockMode(map, gurl));
}

- (void)setDefaultAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode {
  [self setAdBlockMode:adBlockMode forGURL:GURL() isPrivate:false];
}

- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode
                forURL:(NSURL*)url
             isPrivate:(bool)isPrivate {
  GURL gurl = net::GURLWithNSURL(url);
  [self setAdBlockMode:adBlockMode forGURL:gurl isPrivate:isPrivate];
}

- (void)setAdBlockMode:(BraveShieldsAdBlockMode)adBlockMode
               forGURL:(GURL)gurl
             isPrivate:(bool)isPrivate {
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  auto* localState = GetApplicationContext()->GetLocalState();
  auto* profilePrefs = isPrivate
                           ? _profile->GetOffTheRecordProfile()->GetPrefs()
                           : _profile->GetPrefs();
  brave_shields::SetAdBlockMode(
      map, static_cast<brave_shields::mojom::AdBlockMode>(adBlockMode), gurl,
      localState, profilePrefs);
}

@dynamic blockScriptsEnabledByDefault;
- (bool)isBlockScriptsEnabledByDefault {
  return [self blockScriptsEnabledForGURL:GURL() isPrivate:false];
}

- (bool)blockScriptsEnabledForURL:(NSURL*)url isPrivate:(bool)isPrivate {
  GURL gurl = net::GURLWithNSURL(url);
  return [self blockScriptsEnabledForGURL:gurl isPrivate:isPrivate];
}

- (bool)blockScriptsEnabledForGURL:(GURL)gurl isPrivate:(bool)isPrivate {
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  return brave_shields::GetNoScriptEnabled(map, gurl);
}

- (void)setBlockScriptsEnabledByDefault:(bool)isEnabled {
  [self setBlockScriptsEnabled:isEnabled forGURL:GURL() isPrivate:false];
}

- (void)setBlockScriptsEnabled:(bool)isEnabled
                        forURL:(NSURL*)url
                     isPrivate:(bool)isPrivate {
  GURL gurl = net::GURLWithNSURL(url);
  [self setBlockScriptsEnabled:isEnabled forGURL:gurl isPrivate:isPrivate];
}

- (void)setBlockScriptsEnabled:(bool)isEnabled
                       forGURL:(GURL)gurl
                     isPrivate:(bool)isPrivate {
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  auto* localState = GetApplicationContext()->GetLocalState();
  brave_shields::SetIsNoScriptEnabled(map, isEnabled, gurl, localState);
}

@dynamic blockFingerprintingEnabledByDefault;
- (bool)isBlockFingerprintingEnabledByDefault {
  return [self blockFingerprintingEnabledForGURL:GURL() isPrivate:false];
}

- (bool)blockFingerprintingEnabledForURL:(NSURL*)url isPrivate:(bool)isPrivate {
  GURL gurl = net::GURLWithNSURL(url);
  return [self blockFingerprintingEnabledForGURL:gurl isPrivate:isPrivate];
}

- (bool)blockFingerprintingEnabledForGURL:(GURL)gurl isPrivate:(bool)isPrivate {
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  auto fingerprintMode = static_cast<BraveShieldsFingerprintMode>(
      brave_shields::GetFingerprintMode(map, gurl));
  return fingerprintMode != BraveShieldsFingerprintModeAllowMode;
}

- (void)setBlockFingerprintingEnabledByDefault:(bool)isEnabled {
  [self setBlockFingerprintingEnabled:isEnabled forGURL:GURL() isPrivate:false];
}

- (void)setBlockFingerprintingEnabled:(bool)isEnabled
                               forURL:(NSURL*)url
                            isPrivate:(bool)isPrivate {
  GURL gurl = net::GURLWithNSURL(url);
  [self setBlockFingerprintingEnabled:isEnabled
                              forGURL:gurl
                            isPrivate:isPrivate];
}

- (void)setBlockFingerprintingEnabled:(bool)isEnabled
                              forGURL:(GURL)gurl
                            isPrivate:(bool)isPrivate {
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  auto* localState = GetApplicationContext()->GetLocalState();
  auto* profilePrefs = isPrivate
                           ? _profile->GetOffTheRecordProfile()->GetPrefs()
                           : _profile->GetPrefs();
  // iOS only supports standard & allow FingerprintMode's.
  brave_shields::mojom::FingerprintMode fingerprintMode =
      isEnabled ? brave_shields::mojom::FingerprintMode::STANDARD_MODE
                : brave_shields::mojom::FingerprintMode::ALLOW_MODE;
  brave_shields::SetFingerprintMode(map, fingerprintMode, gurl, localState,
                                    profilePrefs);
}

@end
