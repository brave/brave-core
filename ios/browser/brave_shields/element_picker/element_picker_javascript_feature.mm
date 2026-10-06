// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/element_picker/element_picker_javascript_feature.h"

#import <Foundation/Foundation.h>

#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"
#include "url/gurl.h"

namespace {

constexpr char kScriptName[] = "element_picker";
constexpr char kMessageRequestTypeKey[] = "request_type";
constexpr char kMessageDataKey[] = "data";
constexpr char kMessageSelectorKey[] = "selector";

// Request types sent by element_picker_api_webkit.ts.
constexpr char kRequestAddCosmeticFilter[] = "add_cosmetic_filter";
constexpr char kRequestManageCustomFilters[] = "manage_custom_filters";
constexpr char kRequestThemeInfo[] = "theme_info";
constexpr char kRequestLocalizedTexts[] = "localized_texts";

}  // namespace

ElementPickerJavaScriptFeature::ElementPickerJavaScriptFeature()
    : JavaScriptFeature(web::ContentWorld::kIsolatedWorld, {}),
      picker_script_(FeatureScript::CreateWithFilename(
          kScriptName,
          FeatureScript::InjectionTime::kDocumentEnd,
          FeatureScript::TargetFrames::kMainFrame,
          // The script is evaluated directly rather than installed as a user
          // script, so it must not be wrapped in a once-per-window guard --
          // that would stop the picker from being reopened.
          FeatureScript::ReinjectionBehavior::kReinjectOnDocumentRecreation,
          base::BindRepeating(&ElementPickerJavaScriptFeature::GetReplacements,
                              base::Unretained(this)))) {}

ElementPickerJavaScriptFeature::~ElementPickerJavaScriptFeature() = default;

// static
ElementPickerJavaScriptFeature* ElementPickerJavaScriptFeature::GetInstance() {
  static base::NoDestructor<ElementPickerJavaScriptFeature> instance;
  return instance.get();
}

void ElementPickerJavaScriptFeature::ShowPicker(web::WebState* web_state) {
  web::WebFrame* main_frame = GetWebFramesManager(web_state)->GetMainWebFrame();
  if (!main_frame) {
    return;
  }
  ExecuteJavaScript(main_frame,
                    base::SysNSStringToUTF16(picker_script_.GetScriptString()),
                    base::DoNothing());
}

web::JavaScriptFeature::FeatureScript::PlaceholderReplacements
ElementPickerJavaScriptFeature::GetReplacements() {
  NSMutableDictionary* replacements = [[NSMutableDictionary alloc] init];
  [replacements addEntriesFromDictionary:token_.GetPlaceholderReplacements()];
  [replacements
      addEntriesFromDictionary:handler_name_.GetPlaceholderReplacements()];
  return [replacements copy];
}

std::optional<std::string>
ElementPickerJavaScriptFeature::GetScriptMessageHandlerName() const {
  return handler_name_.GetScriptMessageHandlerName();
}

bool ElementPickerJavaScriptFeature::GetFeatureRepliesToPrompts() const {
  return true;
}

bool ElementPickerJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

void ElementPickerJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  const GURL frame_url = message.security_origin().GetURL();
  if (!frame_url.is_valid()) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  auto body = token_.GetValidatedScriptMessageBody(message);
  const base::DictValue* script_dict =
      body ? body.value()->GetIfDict() : nullptr;
  if (!script_dict) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  const std::string* request_type =
      script_dict->FindString(kMessageRequestTypeKey);
  if (!request_type) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  if (*request_type == kRequestAddCosmeticFilter) {
    const base::DictValue* data_dict = script_dict->FindDict(kMessageDataKey);
    const std::string* selector =
        data_dict ? data_dict->FindString(kMessageSelectorKey) : nullptr;
    if (!selector) {
      std::move(callback).Run(nullptr, nil);
      return;
    }
    // TODO: Forward the selector to the tab helper so it can be persisted as a
    // custom filter for `frame_url`.
    std::move(callback).Run(nullptr, nil);
    return;
  } else if (*request_type == kRequestManageCustomFilters) {
    // TODO: Ask the tab helper to present the custom filters settings UI.
    std::move(callback).Run(nullptr, nil);
    return;
  } else if (*request_type == kRequestThemeInfo) {
    // TODO: Reply with `{isDarkModeEnabled, bgcolor}` from the tab helper,
    // which needs the browser's current theme.
    std::move(callback).Run(nullptr, nil);
    return;
  } else if (*request_type == kRequestLocalizedTexts) {
    // TODO: Reply with the picker's button labels. iOS localizes in the Swift
    // layer, so these come across the tab helper bridge.
    std::move(callback).Run(nullptr, nil);
    return;
  }
  std::move(callback).Run(nullptr, nil);
}
