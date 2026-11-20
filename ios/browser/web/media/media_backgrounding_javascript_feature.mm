// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/media/media_backgrounding_javascript_feature.h"

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/to_string.h"
#include "base/values.h"
#include "brave/ios/browser/shared/prefs/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"

namespace {
constexpr char kMediaBackgroundingJavaScriptFeatureKeyName[] =
    "media_backgrounding_java_script_feature";
constexpr char kScriptName[] = "media_backgrounding";
}  // namespace

MediaBackgroundingJavaScriptFeature::MediaBackgroundingJavaScriptFeature(
    ProfileIOS* profile)
    : JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                        /*feature_scripts=*/{}),
      profile_(profile) {}

MediaBackgroundingJavaScriptFeature::~MediaBackgroundingJavaScriptFeature() =
    default;

// static
MediaBackgroundingJavaScriptFeature*
MediaBackgroundingJavaScriptFeature::FromBrowserState(
    web::BrowserState* browser_state) {
  DCHECK(browser_state);
  MediaBackgroundingJavaScriptFeature* feature =
      static_cast<MediaBackgroundingJavaScriptFeature*>(
          browser_state->GetUserData(
              kMediaBackgroundingJavaScriptFeatureKeyName));
  if (!feature) {
    feature = new MediaBackgroundingJavaScriptFeature(
        ProfileIOS::FromBrowserState(browser_state));
    browser_state->SetUserData(kMediaBackgroundingJavaScriptFeatureKeyName,
                               base::WrapUnique(feature));
  }
  return feature;
}

bool MediaBackgroundingJavaScriptFeature::IsMediaBackgroundingEnabled() const {
  return profile_->GetPrefs()->GetBoolean(prefs::kMediaBackgroundingEnabled);
}

std::string MediaBackgroundingJavaScriptFeature::GetScriptsSignature() const {
  return base::ToString(IsMediaBackgroundingEnabled());
}

std::vector<web::JavaScriptFeature::FeatureScript>
MediaBackgroundingJavaScriptFeature::GetScripts() const {
  bool is_media_backgrounding_enabled = IsMediaBackgroundingEnabled();
  return {FeatureScript::CreateWithFilename(
      kScriptName, FeatureScript::InjectionTime::kDocumentStart,
      FeatureScript::TargetFrames::kAllFrames,
      FeatureScript::ReinjectionBehavior::kInjectOncePerWindow,
      base::BindRepeating(
          [](bool is_media_backgrounding_enabled)
              -> FeatureScript::PlaceholderReplacements {
            return @{
              @"window.gCrWebPlaceholderMediaBackgroundingEnabled" :
                      is_media_backgrounding_enabled ? @"true" : @"false"
            };
          },
          is_media_backgrounding_enabled))};
}
