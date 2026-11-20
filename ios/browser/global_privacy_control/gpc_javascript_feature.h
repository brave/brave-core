// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_GLOBAL_PRIVACY_CONTROL_GPC_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_GLOBAL_PRIVACY_CONTROL_GPC_JAVASCRIPT_FEATURE_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/supports_user_data.h"
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
  std::string GetScriptsSignature() const override;

 private:
  bool IsGPCEnabled() const;

  raw_ptr<ProfileIOS> profile_;

  explicit GPCJavaScriptFeature(ProfileIOS* profile);
};

#endif  // BRAVE_IOS_BROWSER_GLOBAL_PRIVACY_CONTROL_GPC_JAVASCRIPT_FEATURE_H_
