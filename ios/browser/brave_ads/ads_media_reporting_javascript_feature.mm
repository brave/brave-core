// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_ads/ads_media_reporting_javascript_feature.h"

#include <optional>

#include "brave/ios/browser/brave_ads/ads_tab_helper.h"
#include "ios/web/public/js_messaging/java_script_feature_util.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"

namespace brave_ads {

namespace {
constexpr char kScriptName[] = "ads_media_reporting";
constexpr char kScriptHandlerName[] = "AdsMediaReportingMessageHandler";
constexpr char kMessageIsPlayingKey[] = "isPlaying";
}  // namespace

AdsMediaReportingJavaScriptFeature::AdsMediaReportingJavaScriptFeature()
    : JavaScriptFeature(web::ContentWorld::kIsolatedWorld,
                        {FeatureScript::CreateWithFilename(
                            kScriptName,
                            FeatureScript::InjectionTime::kDocumentStart,
                            FeatureScript::TargetFrames::kAllFrames,
                            FeatureScript::ReinjectionBehavior::
                                kReinjectOnDocumentRecreation)}) {}

AdsMediaReportingJavaScriptFeature::~AdsMediaReportingJavaScriptFeature() =
    default;

// static
AdsMediaReportingJavaScriptFeature*
AdsMediaReportingJavaScriptFeature::GetInstance() {
  static base::NoDestructor<AdsMediaReportingJavaScriptFeature> instance;
  return instance.get();
}

std::optional<std::string>
AdsMediaReportingJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

void AdsMediaReportingJavaScriptFeature::ScriptMessageReceived(
    web::WebState* web_state,
    const web::ScriptMessage& message) {
  const base::DictValue* script_dict =
      message.body() ? message.body()->GetIfDict() : nullptr;
  if (!script_dict) {
    return;
  }

  std::optional<bool> is_playing = script_dict->FindBool(kMessageIsPlayingKey);
  if (!is_playing) {
    return;
  }

  auto* tab_helper = AdsTabHelper::FromWebState(web_state);
  if (!tab_helper) {
    return;
  }

  if (*is_playing) {
    tab_helper->NotifyTabDidStartPlayingMedia();
  } else {
    tab_helper->NotifyTabDidStopPlayingMedia();
  }
}

}  // namespace brave_ads
