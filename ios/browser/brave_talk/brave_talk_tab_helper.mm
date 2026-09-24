// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_talk/brave_talk_tab_helper.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_talk/pref_names.h"
#include "brave/ios/browser/brave_talk/brave_talk_tab_helper_bridge.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace {

bool IsBraveTalkDisabledByPolicy(PrefService* prefs) {
  return prefs->IsManagedPreference(brave_talk::prefs::kDisabledByPolicy) &&
         prefs->GetBoolean(brave_talk::prefs::kDisabledByPolicy);
}

}  // namespace

BraveTalkTabHelper::BraveTalkTabHelper(web::WebState* web_state)
    : web_state_(web_state) {}

BraveTalkTabHelper::~BraveTalkTabHelper() = default;

void BraveTalkTabHelper::SetBridge(id<BraveTalkTabHelperBridge> bridge) {
  bridge_ = bridge;
}

void BraveTalkTabHelper::LaunchBraveTalk(const std::string_view room,
                                         const std::string_view jwt) {
  PrefService* prefs =
      ProfileIOS::FromBrowserState(web_state_->GetBrowserState())->GetPrefs();
  if (IsBraveTalkDisabledByPolicy(prefs) || !bridge_) {
    return;
  }
  [bridge_ launchBraveTalkWithRoom:base::SysUTF8ToNSString(room)
                            jwtKey:base::SysUTF8ToNSString(jwt)];
}
