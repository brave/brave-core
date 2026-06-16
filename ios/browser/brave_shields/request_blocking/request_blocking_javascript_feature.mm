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
#include "brave/ios/browser/brave_shields/request_blocking/request_blocking_tab_helper.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"
#include "url/gurl.h"

namespace {

constexpr char kScriptName[] = "request_blocking";
constexpr char kScriptHandlerName[] = "RequestBlockingMessageHandler";
constexpr char kMessageResourceTypeKey[] = "resourceType";
constexpr char kMessageResourceURLKey[] = "resourceURL";

}  // namespace

RequestBlockingJavaScriptFeature::RequestBlockingJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentStart,
              FeatureScript::TargetFrames::kAllFrames,
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

  const base::DictValue* script_dict =
      message.body() ? message.body()->GetIfDict() : nullptr;
  if (!script_dict) {
    std::move(reply_handler).Run(false);
    return;
  }

  const std::string* resource_type_string =
      script_dict->FindString(kMessageResourceTypeKey);
  const std::string* resource_url_string =
      script_dict->FindString(kMessageResourceURLKey);
  if (!resource_type_string || !resource_url_string) {
    std::move(reply_handler).Run(false);
    return;
  }

  const GURL resource_url(*resource_url_string);
  if (!resource_url.is_valid()) {
    std::move(reply_handler).Run(false);
    return;
  }

  auto* tab_helper = RequestBlockingTabHelper::FromWebState(web_state);
  if (!tab_helper) {
    std::move(reply_handler).Run(false);
    return;
  }

  // TODO(https://github.com/brave/brave-browser/issues/56116)
  // Use security_origin
  // https://source.chromium.org/chromium/chromium/src/+/f25be9b3246ed08b4b853435b40eb3380dc0e7bb
  const GURL window_origin_url = message.request_url().value_or(GURL());
  if (!window_origin_url.is_valid()) {
    std::move(reply_handler).Run(false);
    return;
  }

  tab_helper->ShouldBlock(resource_url, window_origin_url,
                          *resource_type_string, std::move(reply_handler));
}
