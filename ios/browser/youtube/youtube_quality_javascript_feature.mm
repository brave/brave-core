// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/youtube/youtube_quality_javascript_feature.h"

#include <optional>
#include <string>

#include "base/containers/fixed_flat_set.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "brave/ios/browser/youtube/pref_names.h"
#include "brave/ios/browser/youtube/youtube_network_change_observer.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"
#include "net/base/network_change_notifier.h"
#include "url/gurl.h"

namespace youtube {

namespace {

constexpr char kScriptName[] = "yt_video_quality";
constexpr char kEventListenersScriptName[] = "yt_video_quality_event_listeners";
constexpr char kScriptHandlerName[] = "YouTubeQualityMessageHandler";
constexpr auto kAllowedHosts = base::MakeFixedFlatSet<std::string_view>(
    {"m.youtube.com", "www.youtube.com", "youtube.com"});

}  // namespace

YouTubeQualityJavaScriptFeature::YouTubeQualityJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {FeatureScript::CreateWithFilename(
               kScriptName,
               FeatureScript::InjectionTime::kDocumentStart,
               FeatureScript::TargetFrames::kMainFrame,
               FeatureScript::ReinjectionBehavior::kInjectOncePerWindow),
           FeatureScript::CreateWithFilename(
               kEventListenersScriptName,
               FeatureScript::InjectionTime::kDocumentStart,
               FeatureScript::TargetFrames::kMainFrame,
               FeatureScript::ReinjectionBehavior::
                   kReinjectOnDocumentRecreation)}) {}

YouTubeQualityJavaScriptFeature::~YouTubeQualityJavaScriptFeature() = default;

// static
YouTubeQualityJavaScriptFeature*
YouTubeQualityJavaScriptFeature::GetInstance() {
  static base::NoDestructor<YouTubeQualityJavaScriptFeature> instance;
  return instance.get();
}

void YouTubeQualityJavaScriptFeature::ResetQuality(web::WebState* web_state) {
  web::WebFrame* main_frame = GetWebFramesManager(web_state)->GetMainWebFrame();
  if (!main_frame) {
    return;
  }
  CallJavaScriptFunction(main_frame, "youtubeQuality.resetQuality",
                         base::ListValue());
}

bool YouTubeQualityJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

std::optional<std::string>
YouTubeQualityJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

void YouTubeQualityJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  GURL request_url = message.request_url().value_or(GURL());

  if (!message.is_main_frame() || !request_url.is_valid() ||
      !kAllowedHosts.contains(request_url.host())) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  PrefService* prefs =
      ProfileIOS::FromBrowserState(web_state->GetBrowserState())->GetPrefs();
  AutoQualityMode mode = static_cast<youtube::AutoQualityMode>(
      prefs->GetInteger(prefs::kAutoQualityMode));
  bool should_set_highest_quality =
      mode == youtube::AutoQualityMode::kOn ||
      (mode == youtube::AutoQualityMode::kWifiOnly &&
       !net::NetworkChangeNotifier::IsConnectionCellular(
           net::NetworkChangeNotifier::GetConnectionType()));

  const base::Value reply(should_set_highest_quality);
  std::move(callback).Run(&reply, nil);
}

}  // namespace youtube
