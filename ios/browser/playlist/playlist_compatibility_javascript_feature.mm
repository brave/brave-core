// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/playlist/playlist_compatibility_javascript_feature.h"

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "brave/ios/browser/playlist/playlist_compatibility_flag_data.h"
#include "brave/ios/web/js_messaging/message_handler_token.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"

namespace playlist {

namespace {

constexpr char kScriptName[] = "playlist_compatibility";
constexpr char kScriptHandlerName[] = "PlaylistCompatibilityMessageHandler";

}  // namespace

PlaylistCompatibilityJavaScriptFeature::PlaylistCompatibilityJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentStart,
              FeatureScript::TargetFrames::kAllFrames,
              FeatureScript::ReinjectionBehavior::kInjectOncePerWindow,
              base::BindRepeating(
                  &web::MessageHandlerToken::GetPlaceholderReplacements,
                  base::Unretained(&token_)))}) {}

PlaylistCompatibilityJavaScriptFeature::
    ~PlaylistCompatibilityJavaScriptFeature() = default;

// static
PlaylistCompatibilityJavaScriptFeature*
PlaylistCompatibilityJavaScriptFeature::GetInstance() {
  static base::NoDestructor<PlaylistCompatibilityJavaScriptFeature> instance;
  return instance.get();
}

bool PlaylistCompatibilityJavaScriptFeature::GetFeatureRepliesToPrompts()
    const {
  return true;
}

std::optional<std::string>
PlaylistCompatibilityJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

void PlaylistCompatibilityJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  if (!token_.GetValidatedScriptMessageBody(message)) {
    base::Value value(false);
    std::move(callback).Run(&value, nullptr);
    return;
  }
  // Check if compatiblity marker is in web_state and return that value
  bool is_compatibility_mode_enabled =
      PlaylistCompatibilityFlagData::FromWebState(web_state) != nullptr;
  base::Value value(is_compatibility_mode_enabled);
  std::move(callback).Run(&value, nullptr);
}

}  // namespace playlist
