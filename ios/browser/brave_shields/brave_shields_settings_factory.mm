// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/brave_shields_settings_factory.h"

#include "base/apple/foundation_util.h"
#include "brave/components/brave_shields/ios/browser/brave_shields_settings.h"
#include "brave/components/brave_shields/ios/browser/brave_shields_settings_impl.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

@implementation BraveShieldsSettingsFactory

+ (id<BraveShieldsSettings>)getForProfile:(id<ProfileBridge>)profile {
  ProfileBridgeImpl* holder =
      base::apple::ObjCCastStrict<ProfileBridgeImpl>(profile);
  ProfileIOS* _profile = holder.profile;
  auto* map = ios::HostContentSettingsMapFactory::GetForProfile(_profile);
  auto* localState = GetApplicationContext()->GetLocalState();
  auto* profilePrefs = _profile->GetPrefs();
  return [[BraveShieldsSettingsImpl alloc]
      initWithHostContentSettingsMap:map
                          localState:localState
                        profilePrefs:profilePrefs];
}

@end
