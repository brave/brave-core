// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/global_privacy_control/gpc_javascript_feature.h"

#include "base/functional/bind.h"
#include "base/strings/to_string.h"
#include "brave/components/global_privacy_control/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/web_frame.h"

namespace {
constexpr char kGPCJavaScriptFeatureKeyName[] = "gpc_java_script_feature";
constexpr char kScriptName[] = "gpc";
}  // namespace

GPCJavaScriptFeature::GPCJavaScriptFeature(ProfileIOS* profile)
    : JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                        /*feature_scripts=*/{}),
      profile_(profile) {}

GPCJavaScriptFeature::~GPCJavaScriptFeature() = default;

// static
GPCJavaScriptFeature* GPCJavaScriptFeature::FromBrowserState(
    web::BrowserState* browser_state) {
  DCHECK(browser_state);

  GPCJavaScriptFeature* feature = static_cast<GPCJavaScriptFeature*>(
      browser_state->GetUserData(kGPCJavaScriptFeatureKeyName));
  if (!feature) {
    feature =
        new GPCJavaScriptFeature(ProfileIOS::FromBrowserState(browser_state));
    browser_state->SetUserData(kGPCJavaScriptFeatureKeyName,
                               base::WrapUnique(feature));
  }
  return feature;
}

bool GPCJavaScriptFeature::IsGPCEnabled() const {
  return profile_->GetPrefs()->GetBoolean(
      global_privacy_control::kGlobalPrivacyControlEnabled);
}

std::string GPCJavaScriptFeature::GetScriptsSignature() const {
  return base::ToString(IsGPCEnabled());
}

std::vector<web::JavaScriptFeature::FeatureScript>
GPCJavaScriptFeature::GetScripts() const {
  bool is_gpc_enabled = IsGPCEnabled();
  return {FeatureScript::CreateWithFilename(
      kScriptName, FeatureScript::InjectionTime::kDocumentStart,
      FeatureScript::TargetFrames::kAllFrames,
      FeatureScript::ReinjectionBehavior::kInjectOncePerWindow,
      base::BindRepeating(
          [](bool is_gpc_enabled) -> FeatureScript::PlaceholderReplacements {
            return @{
              @"window.gCrWebPlaceholderGPCEnabled" : is_gpc_enabled ? @"true"
                                                                     : @"false"
            };
          },
          is_gpc_enabled))};
}
