// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_javascript_feature.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

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

constexpr char kScriptName[] = "cosmetic_filtering";
constexpr char kScriptHandlerName[] = "CosmeticFilteringMessageHandler";

}  // namespace

CosmeticFilteringJavaScriptFeature::CosmeticFilteringJavaScriptFeature()
    : JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                        {FeatureScript::CreateWithFilename(
                            kScriptName,
                            FeatureScript::InjectionTime::kDocumentStart,
                            FeatureScript::TargetFrames::kAllFrames,
                            FeatureScript::ReinjectionBehavior::
                                kReinjectOnDocumentRecreation)}) {}

CosmeticFilteringJavaScriptFeature::~CosmeticFilteringJavaScriptFeature() =
    default;

// static
CosmeticFilteringJavaScriptFeature*
CosmeticFilteringJavaScriptFeature::GetInstance() {
  static base::NoDestructor<CosmeticFilteringJavaScriptFeature> instance;
  return instance.get();
}

std::optional<std::string>
CosmeticFilteringJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

bool CosmeticFilteringJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

void CosmeticFilteringJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  auto reply_handler = base::BindOnce(
      [](ScriptMessageReplyCallback handler,
         std::vector<std::string> standard_selectors,
         std::vector<std::string> aggressive_selectors) {
        base::ListValue standard_list;
        for (auto& selector : standard_selectors) {
          standard_list.Append(std::move(selector));
        }

        base::ListValue aggressive_list;
        for (auto& selector : aggressive_selectors) {
          aggressive_list.Append(std::move(selector));
        }

        base::DictValue response;
        response.Set("standard_selectors", std::move(standard_list));
        response.Set("aggressive_selectors", std::move(aggressive_list));

        base::Value value(std::move(response));
        std::move(handler).Run(&value, nil);
      },
      std::move(callback));

  std::move(reply_handler).Run({}, {});
  return;
}
