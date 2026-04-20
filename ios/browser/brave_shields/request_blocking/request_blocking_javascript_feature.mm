// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/request_blocking/request_blocking_javascript_feature.h"

#include <optional>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/navigation/navigation_item.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "url/gurl.h"

namespace {

constexpr char kScriptName[] = "request_blocking";
constexpr char kScriptHandlerName[] = "RequestBlockingMessageHandler";

}  // namespace

RequestBlockingJavaScriptFeature::RequestBlockingJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentStart,
              FeatureScript::TargetFrames::kMainFrame,
              FeatureScript::ReinjectionBehavior::kInjectOncePerWindow)}) {}

RequestBlockingJavaScriptFeature::~RequestBlockingJavaScriptFeature() = default;

// static
RequestBlockingJavaScriptFeature*
RequestBlockingJavaScriptFeature::GetInstance() {
  static base::NoDestructor<RequestBlockingJavaScriptFeature> instance;
  return instance.get();
}

std::optional<std::string>
RequestBlockingJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

bool RequestBlockingJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

void RequestBlockingJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  auto reply_handler = base::BindOnce(
      [](ScriptMessageReplyCallback handler, bool should_redirect) {
        base::Value value(should_redirect);
        std::move(handler).Run(&value, nil);
      },
      std::move(callback));

  std::move(reply_handler).Run(false);
  return;
}
