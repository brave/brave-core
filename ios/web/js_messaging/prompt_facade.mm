// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/js_messaging/prompt_facade.h"

#include <memory>
#include <optional>
#include <string>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "ios/web/js_messaging/java_script_content_world.h"
#include "ios/web/js_messaging/java_script_feature_manager.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/web_state.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace web {

namespace {

// Expected JSON key names when handling prompts from JavaScriptFeatures
constexpr char kHandlerNameKey[] = "handler";
constexpr char kMessageKey[] = "message";

// Get the first JavaScriptFeature that replies to prompts with the given
// handler name. Unfortunately prompts cannot supply their current content world
// so we must check all of them.
const JavaScriptFeature* GetFeatureReplyingToPrompts(
    web::BrowserState* browser_state,
    const std::string& handler_name) {
  std::vector<JavaScriptContentWorld*> worlds =
      JavaScriptFeatureManager::FromBrowserState(browser_state)
          ->GetAllContentWorlds();
  for (JavaScriptContentWorld* world : worlds) {
    if (const JavaScriptFeature* found =
            world->GetFeatureReplyingToPrompts(handler_name)) {
      return found;
    }
  }
  return nullptr;
}

}  // namespace

PromptFacade::PromptFacade(web::WebState* web_state) : web_state_(web_state) {}

std::optional<std::string> PromptFacade::HandleJavaScriptPrompt(
    GURL request_url,
    url::Origin security_origin,
    bool is_main_frame,
    const std::string& prompt) {
  if (!request_url.is_valid()) {
    return std::nullopt;
  }

  std::optional<base::DictValue> dict = base::JSONReader::ReadDict(
      prompt, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!dict) {
    return std::nullopt;
  }

  const std::string* handler_name = dict->FindString(kHandlerNameKey);
  const base::Value* message = dict->Find(kMessageKey);
  if (!handler_name || !message) {
    return std::nullopt;
  }

  // Typically upstream binds `ScriptMessageReceivedWithReply` to the mutable
  // `WeakPtr` of `JavaScriptFeature` which removes the `const`. Since our code
  // is non-escaping though we don't need to make a `WeakPtr` so must
  // `const_cast`
  JavaScriptFeature* feature =
      const_cast<JavaScriptFeature*>(GetFeatureReplyingToPrompts(
          web_state_->GetBrowserState(), *handler_name));

  if (!feature || !feature->ShouldHandleMessageFromOrigin(security_origin)) {
    return std::nullopt;
  }

  ScriptMessage script_message(std::make_unique<base::Value>(message->Clone()),
                               /*is_user_interacting=*/false, is_main_frame,
                               std::move(request_url), security_origin);

  bool replied = false;
  std::string reply_json;
  feature->ScriptMessageReceivedWithReply(
      web_state_, script_message,
      base::BindOnce(
          [](bool* replied, std::string* reply_json, const base::Value* reply,
             NSString* error) {
            *replied = true;
            if (reply) {
              base::JSONWriter::Write(*reply, reply_json);
            }
          },
          &replied, &reply_json));
  DCHECK(replied)
      << "JavaScriptFeature replying to window.prompt must reply synchronously";
  return reply_json;
}

}  // namespace web
