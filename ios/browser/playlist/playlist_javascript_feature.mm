// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/playlist/playlist_javascript_feature.h"

#include <optional>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/playlist/core/common/features.h"
#include "brave/components/playlist/core/common/pref_names.h"
#include "brave/ios/browser/playlist/playlist_tab_helper.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"

namespace playlist {

namespace {
constexpr char kScriptName[] = "playlist";
constexpr char kEventListenersScriptName[] = "playlist_event_listeners";
constexpr char kScriptHandlerName[] = "PlaylistMessageHandler";

// JavaScript functions exposed on the `playlist` API in playlist.ts.
constexpr char kGetCurrentTimeFunction[] =
    "playlist.getCurrentTimeForVideoWithTag";
constexpr char kLongPressedAtLocationFunction[] =
    "playlist.longPressedAtLocation";

// Time allowed for the page to reply to a getCurrentTimeForVideoWithTag query.
constexpr base::TimeDelta kFunctionCallTimeout = base::Seconds(5);

bool IsPlaylistAvailable(PrefService* prefs) {
  bool is_disabled_by_policy =
      prefs->IsManagedPreference(kPlaylistEnabledPref) &&
      !prefs->GetBoolean(kPlaylistEnabledPref);
  return base::FeatureList::IsEnabled(features::kPlaylist) &&
         !is_disabled_by_policy;
}

}  // namespace

PlaylistJavaScriptFeature::PlaylistJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {FeatureScript::CreateWithFilename(
               kScriptName,
               FeatureScript::InjectionTime::kDocumentStart,
               FeatureScript::TargetFrames::kAllFrames,
               FeatureScript::ReinjectionBehavior::kInjectOncePerWindow),
           FeatureScript::CreateWithFilename(
               kEventListenersScriptName,
               FeatureScript::InjectionTime::kDocumentStart,
               FeatureScript::TargetFrames::kAllFrames,
               FeatureScript::ReinjectionBehavior::
                   kReinjectOnDocumentRecreation)}) {}

PlaylistJavaScriptFeature::~PlaylistJavaScriptFeature() = default;

// static
PlaylistJavaScriptFeature* PlaylistJavaScriptFeature::GetInstance() {
  static base::NoDestructor<PlaylistJavaScriptFeature> instance;
  return instance.get();
}

void PlaylistJavaScriptFeature::GetCurrentTimeForVideoWithTag(
    web::WebState* web_state,
    const std::string& tag,
    base::OnceCallback<void(double)> callback) {
  web::WebFrame* main_frame = GetWebFramesManager(web_state)->GetMainWebFrame();
  if (!main_frame) {
    std::move(callback).Run(0.0);
    return;
  }

  CallJavaScriptFunction(main_frame, kGetCurrentTimeFunction,
                         base::ListValue().Append(tag),
                         base::BindOnce(
                             [](base::OnceCallback<void(double)> callback,
                                const base::Value* value) {
                               std::optional<double> result =
                                   value ? value->GetIfDouble() : std::nullopt;
                               std::move(callback).Run(result.value_or(0.0));
                             },
                             std::move(callback)),
                         kFunctionCallTimeout);
}

void PlaylistJavaScriptFeature::LongPressedAtLocation(web::WebState* web_state,
                                                      double x,
                                                      double y) {
  web::WebFrame* main_frame = GetWebFramesManager(web_state)->GetMainWebFrame();
  if (!main_frame) {
    return;
  }

  CallJavaScriptFunction(main_frame, kLongPressedAtLocationFunction,
                         base::ListValue().Append(x).Append(y));
}

std::optional<std::string>
PlaylistJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

void PlaylistJavaScriptFeature::ScriptMessageReceived(
    web::WebState* web_state,
    const web::ScriptMessage& message) {
  PrefService* prefs =
      ProfileIOS::FromBrowserState(web_state->GetBrowserState())->GetPrefs();
  if (!IsPlaylistAvailable(prefs)) {
    return;
  }

  const base::DictValue* item =
      message.body() ? message.body()->GetIfDict() : nullptr;
  if (!item) {
    return;
  }

  PlaylistTabHelper* tab_helper = PlaylistTabHelper::FromWebState(web_state);
  if (!tab_helper) {
    return;
  }
  tab_helper->OnMediaDetected(*item);
}

}  // namespace playlist
