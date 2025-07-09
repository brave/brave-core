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

@interface BraveShieldsUtilsIOS () {
}

@end

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

- (bool)getBraveShieldsEnabledFor:(NSURL*)url isPrivate:(bool)isPrivate {
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
  auto* local_state = GetApplicationContext()->GetLocalState();
  brave_shields::SetBraveShieldsEnabled(map, isEnabled, gurl, local_state);
}

- (BraveShieldsAdBlockMode)getDefaultAdBlockMode {
  return [self getAdBlockModeForGURL:GURL() isPrivate:false];
}

- (BraveShieldsAdBlockMode)getAdBlockModeForURL:(NSURL*)url
                                      isPrivate:(bool)isPrivate {
  GURL gurl = net::GURLWithNSURL(url);
  return [self getAdBlockModeForGURL:gurl isPrivate:isPrivate];
}

- (BraveShieldsAdBlockMode)getAdBlockModeForGURL:(GURL)gurl
                                       isPrivate:(bool)isPrivate {
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];

  brave_shields::ControlType control_type_ad =
      brave_shields::GetAdControlType(map, gurl);

  brave_shields::ControlType control_type_cosmetic =
      brave_shields::GetCosmeticFilteringControlType(map, gurl);

  if (control_type_ad == brave_shields::ControlType::ALLOW) {
    return BraveShieldsAdBlockModeAllow;
  }

  if (control_type_cosmetic == brave_shields::ControlType::BLOCK) {
    return BraveShieldsAdBlockModeAggressive;
  } else {
    return BraveShieldsAdBlockModeStandard;
  }
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
  brave_shields::ControlType control_type_cosmetic;
  switch (adBlockMode) {
    case BraveShieldsAdBlockModeAggressive:
      control_type_cosmetic = brave_shields::ControlType::BLOCK;  // aggressive
      break;
    case BraveShieldsAdBlockModeStandard:
      control_type_cosmetic =
          brave_shields::ControlType::BLOCK_THIRD_PARTY;  // standard
      break;
    case BraveShieldsAdBlockModeAllow:
      control_type_cosmetic = brave_shields::ControlType::ALLOW;  // allow
      break;
  }
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  auto* local_state = GetApplicationContext()->GetLocalState();
  auto* profile_prefs = _profile->GetPrefs();

  brave_shields::SetCosmeticFilteringControlType(
      map, control_type_cosmetic, gurl, local_state, profile_prefs);
}

- (bool)getBlockScriptsEnabledByDefault {
  return [self getBlockScriptsEnabledForGURL:GURL() isPrivate:false];
}

- (bool)getBlockScriptsEnabledForURL:(NSURL*)url isPrivate:(bool)isPrivate {
  GURL gurl = net::GURLWithNSURL(url);
  return [self getBlockScriptsEnabledForGURL:gurl isPrivate:isPrivate];
}

- (bool)getBlockScriptsEnabledForGURL:(GURL)gurl isPrivate:(bool)isPrivate {
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  brave_shields::ControlType control_type =
      brave_shields::GetNoScriptControlType(map, gurl);

  if (control_type == brave_shields::ControlType::ALLOW) {
    return false;
  }

  return true;
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
  brave_shields::ControlType control_type;

  if (!isEnabled) {
    control_type = brave_shields::ControlType::ALLOW;
  } else {
    control_type = brave_shields::ControlType::BLOCK;
  }
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  auto* local_state = GetApplicationContext()->GetLocalState();

  brave_shields::SetNoScriptControlType(map, control_type, gurl, local_state);
}

- (bool)getBlockFingerprintingEnabledByDefault {
  return [self getBlockFingerprintingEnabledForGURL:GURL() isPrivate:false];
}

- (bool)getBlockFingerprintingEnabledForURL:(NSURL*)url
                                  isPrivate:(bool)isPrivate {
  GURL gurl = net::GURLWithNSURL(url);
  return [self getBlockFingerprintingEnabledForGURL:gurl isPrivate:isPrivate];
}

- (bool)getBlockFingerprintingEnabledForGURL:(GURL)gurl
                                   isPrivate:(bool)isPrivate {
  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  brave_shields::ControlType control_type =
      brave_shields::GetFingerprintingControlType(map, gurl);

  if (control_type == brave_shields::ControlType::ALLOW) {
    return false;
  } else if (control_type == brave_shields::ControlType::BLOCK) {
    return true;
  } else {  // brave_shields::ControlType::DEFAULT
    return true;
  }
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
  brave_shields::ControlType control_type;

  if (isEnabled) {
    control_type = brave_shields::ControlType::DEFAULT;  // STANDARD_MODE
  } else {
    // ControlType::ALLOW to allow fingerprinting
    control_type = brave_shields::ControlType::ALLOW;
  }

  HostContentSettingsMap* map =
      [self hostContentSettingsMapForPrivate:isPrivate];
  auto* local_state = GetApplicationContext()->GetLocalState();
  auto* profile_prefs = _profile->GetPrefs();
  brave_shields::SetFingerprintingControlType(map, control_type, gurl,
                                              local_state, profile_prefs);
}

@end
