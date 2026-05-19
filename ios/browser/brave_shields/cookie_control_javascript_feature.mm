// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/cookie_control_javascript_feature.h"

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "brave/ios/browser/shared/prefs/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"
#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"

namespace {
constexpr char kCookieControlJavaScriptFeatureKeyName[] =
    "cookie_control_java_script_feature";
constexpr char kScriptName[] = "cookie_control";
}  // namespace

CookieControlJavaScriptFeature::CookieControlJavaScriptFeature(
    ProfileIOS* profile)
    : JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                        /*feature_scripts=*/{}),
      profile_(profile) {
  prefs_change_registrar_.Init(profile_->GetPrefs());
  prefs_change_registrar_.Add(
      prefs::kBlockAllCookiesEnabled,
      base::BindRepeating(&CookieControlJavaScriptFeature::OnPrefUpdated,
                          base::Unretained(this)));
}

CookieControlJavaScriptFeature::~CookieControlJavaScriptFeature() = default;

// static
CookieControlJavaScriptFeature*
CookieControlJavaScriptFeature::FromBrowserState(
    web::BrowserState* browser_state) {
  DCHECK(browser_state);
  CookieControlJavaScriptFeature* feature =
      static_cast<CookieControlJavaScriptFeature*>(
          browser_state->GetUserData(kCookieControlJavaScriptFeatureKeyName));
  if (!feature) {
    feature = new CookieControlJavaScriptFeature(
        ProfileIOS::FromBrowserState(browser_state));
    browser_state->SetUserData(kCookieControlJavaScriptFeatureKeyName,
                               base::WrapUnique(feature));
  }
  return feature;
}

void CookieControlJavaScriptFeature::OnPrefUpdated() {
  // Feature scripts must be explicitly updated after this pref changes.
  web::WKWebViewConfigurationProvider& config_provider =
      web::WKWebViewConfigurationProvider::FromBrowserState(profile_);
  config_provider.UpdateScripts();
}

std::vector<web::JavaScriptFeature::FeatureScript>
CookieControlJavaScriptFeature::GetScripts() const {
  bool is_block_all_cookies_enabled =
      profile_->GetPrefs()->GetBoolean(prefs::kBlockAllCookiesEnabled);
  return {FeatureScript::CreateWithFilename(
      kScriptName, FeatureScript::InjectionTime::kDocumentStart,
      FeatureScript::TargetFrames::kAllFrames,
      FeatureScript::ReinjectionBehavior::kInjectOncePerWindow,
      base::BindRepeating(
          [](bool is_block_all_cookies_enabled)
              -> FeatureScript::PlaceholderReplacements {
            return @{
              @"window.gCrWebPlaceholderBlockAllCookiesEnabled" :
                      is_block_all_cookies_enabled ? @"true" : @"false"
            };
          },
          is_block_all_cookies_enabled))};
}
