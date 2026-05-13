// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/farbling_javascript_feature.h"

#include "base/functional/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace brave_shields {

namespace {
constexpr char kScriptName[] = "farbling";
}  // namespace

FarblingJavaScriptFeature::FarblingJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {web::JavaScriptFeature::FeatureScript::CreateWithFilename(
              kScriptName,
              web::JavaScriptFeature::FeatureScript::InjectionTime::
                  kDocumentStart,
              web::JavaScriptFeature::FeatureScript::TargetFrames::kAllFrames,
              web::JavaScriptFeature::FeatureScript::ReinjectionBehavior::
                  kReinjectOnDocumentRecreation,
              base::BindRepeating(&FarblingJavaScriptFeature::GetReplacements,
                                  base::Unretained(this)))}) {}

FarblingJavaScriptFeature::~FarblingJavaScriptFeature() = default;

// static
FarblingJavaScriptFeature* FarblingJavaScriptFeature::GetInstance() {
  static base::NoDestructor<FarblingJavaScriptFeature> instance;
  return instance.get();
}

std::optional<std::string>
FarblingJavaScriptFeature::GetScriptMessageHandlerName() const {
  return handler_name_.GetScriptMessageHandlerName();
}

FarblingJavaScriptFeature::FeatureScript::PlaceholderReplacements
FarblingJavaScriptFeature::GetReplacements() {
  return handler_name_.GetPlaceholderReplacements();
}

bool FarblingJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

bool FarblingJavaScriptFeature::GetFeatureRepliesToPrompts() const {
  return true;
}

void FarblingJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  base::Value reply("{\"data\":\"hello, world\"}");
  std::move(callback).Run(&reply, nil);
}

}  // namespace brave_shields
