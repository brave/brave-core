// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/user_scripts/user_scripts_javascript_feature.h"

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/values.h"
#include "ios/web/public/js_messaging/script_message.h"

namespace {
constexpr char kScriptName[] = "user_scripts";
}  // namespace

UserScriptsJavaScriptFeature::UserScriptsJavaScriptFeature()
    : ProtectedJavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentEnd,
              FeatureScript::TargetFrames::kAllFrames,
              FeatureScript::ReinjectionBehavior::kReinjectOnDocumentRecreation,
              base::BindRepeating(
                  &UserScriptsJavaScriptFeature::GetPlaceholderReplacements,
                  base::Unretained(this)))}) {}

UserScriptsJavaScriptFeature::~UserScriptsJavaScriptFeature() = default;

// static
UserScriptsJavaScriptFeature* UserScriptsJavaScriptFeature::GetInstance() {
  static base::NoDestructor<UserScriptsJavaScriptFeature> instance;
  return instance.get();
}

void UserScriptsJavaScriptFeature::ScriptMessageReceived(
    web::WebState* web_state,
    const web::ScriptMessage& message) {
  std::optional<const base::Value*> body =
      GetValidatedScriptMessageBody(message);
  LOG(INFO) << "UserScriptsJavaScriptFeature::ScriptMessageReceived "
            << (body.has_value() ? body.value()->DebugString() : "null");
}
