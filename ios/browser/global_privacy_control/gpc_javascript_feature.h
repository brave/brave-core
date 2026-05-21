// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_GLOBAL_PRIVACY_CONTROL_GPC_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_GLOBAL_PRIVACY_CONTROL_GPC_JAVASCRIPT_FEATURE_H_

#include "base/memory/raw_ptr.h"
#include "components/prefs/pref_change_registrar.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

class ProfileIOS;

class GPCJavaScriptFeature : public web::JavaScriptFeature,
                             public base::SupportsUserData::Data {
 public:
  ~GPCJavaScriptFeature() override;

  static GPCJavaScriptFeature* FromBrowserState(
      web::BrowserState* browser_state);

  // web::JavaScriptFeature
  std::vector<web::JavaScriptFeature::FeatureScript> GetScripts()
      const override;

 private:
  raw_ptr<ProfileIOS> profile_;
  PrefChangeRegistrar prefs_change_registrar_;

  void GPCPrefUpdated();

  explicit GPCJavaScriptFeature(ProfileIOS* profile);
};

#endif  // BRAVE_IOS_BROWSER_GLOBAL_PRIVACY_CONTROL_GPC_JAVASCRIPT_FEATURE_H_
