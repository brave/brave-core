// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/logins/logins_javascript_feature.h"

#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/ios/browser/web/logins/logins_tab_helper.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/web_state.h"
#include "url/gurl.h"

namespace {

constexpr char kScriptName[] = "logins";
constexpr char kScriptHandlerName[] = "LoginsMessageHandler";
constexpr char kMessageTypeKey[] = "type";
constexpr char kMessageTypeRequest[] = "request";
constexpr char kMessageTypeSubmit[] = "submit";
constexpr char kActionOriginKey[] = "actionOrigin";
constexpr char kSubmitDataKey[] = "data";

}  // namespace

LoginsJavaScriptFeature::LoginsJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kIsolatedWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentStart,
              FeatureScript::TargetFrames::kAllFrames,
              FeatureScript::ReinjectionBehavior::kInjectOncePerWindow)}) {}

LoginsJavaScriptFeature::~LoginsJavaScriptFeature() = default;

// static
LoginsJavaScriptFeature* LoginsJavaScriptFeature::GetInstance() {
  static base::NoDestructor<LoginsJavaScriptFeature> instance;
  return instance.get();
}

std::optional<std::string>
LoginsJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

bool LoginsJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

void LoginsJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  const base::DictValue* body =
      message.body() ? message.body()->GetIfDict() : nullptr;
  if (!body) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  const std::string* type = body->FindString(kMessageTypeKey);
  if (!type) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  auto* tab_helper = LoginsTabHelper::FromWebState(web_state);
  if (!tab_helper) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  if (*type == kMessageTypeRequest) {
    const std::optional<GURL> request_url = message.request_url();
    const std::string* action_origin = body->FindString(kActionOriginKey);
    if (!request_url || !action_origin) {
      std::move(callback).Run(nullptr, nil);
      return;
    }

    tab_helper->GetSavedLogins(
        *request_url, *action_origin, message.is_main_frame(),
        base::BindOnce(
            [](ScriptMessageReplyCallback handler, std::string json) {
              auto parsed = base::JSONReader::Read(json, base::JSON_PARSE_RFC);
              std::move(handler).Run(parsed ? &parsed.value() : nullptr, nil);
            },
            std::move(callback)));
  } else if (*type == kMessageTypeSubmit) {
    const std::string* json = body->FindString(kSubmitDataKey);
    if (!json) {
      std::move(callback).Run(nullptr, nil);
      return;
    }
    tab_helper->HandleFormSubmit(*json);
    std::move(callback).Run(nullptr, nil);
  } else {
    std::move(callback).Run(nullptr, nil);
  }
}
