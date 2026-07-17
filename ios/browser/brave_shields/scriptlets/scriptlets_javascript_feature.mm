// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/scriptlets/scriptlets_javascript_feature.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/ios/browser/brave_shields/scriptlets/scriptlets_tab_helper.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/web_state.h"
#include "url/gurl.h"

namespace {

constexpr char kScriptName[] = "scriptlets";

}  // namespace

ScriptletsJavaScriptFeature::ScriptletsJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentStart,
              FeatureScript::TargetFrames::kAllFrames,
              FeatureScript::ReinjectionBehavior::kReinjectOnDocumentRecreation,
              base::BindRepeating(&ScriptletsJavaScriptFeature::GetReplacements,
                                  base::Unretained(this)))}) {}

ScriptletsJavaScriptFeature::~ScriptletsJavaScriptFeature() = default;

// static
ScriptletsJavaScriptFeature* ScriptletsJavaScriptFeature::GetInstance() {
  static base::NoDestructor<ScriptletsJavaScriptFeature> instance;
  return instance.get();
}

web::JavaScriptFeature::FeatureScript::PlaceholderReplacements
ScriptletsJavaScriptFeature::GetReplacements() {
  NSMutableDictionary* replacements = [[NSMutableDictionary alloc] init];
  [replacements addEntriesFromDictionary:token_.GetPlaceholderReplacements()];
  [replacements
      addEntriesFromDictionary:handler_name_.GetPlaceholderReplacements()];
  return [replacements copy];
}

std::optional<std::string>
ScriptletsJavaScriptFeature::GetScriptMessageHandlerName() const {
  return handler_name_.GetScriptMessageHandlerName();
}

bool ScriptletsJavaScriptFeature::GetFeatureRepliesToPrompts() const {
  return true;
}

bool ScriptletsJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

void ScriptletsJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  auto reply_handler = base::BindOnce(
      [](ScriptMessageReplyCallback handler,
         std::vector<std::string> scriptlets) {
        base::ListValue list;
        for (auto& scriptlet : scriptlets) {
          list.Append(std::move(scriptlet));
        }
        base::Value value(std::move(list));
        std::move(handler).Run(&value, nil);
      },
      std::move(callback));

  auto* tab_helper = ScriptletsTabHelper::FromWebState(web_state);
  if (!tab_helper) {
    std::move(reply_handler).Run({});
    return;
  }

  const GURL frame_url = message.security_origin().GetURL();
  tab_helper->RequestScriptlets(frame_url, std::move(reply_handler));
}
