// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "brave/ios/web/js_messaging/js_prompt_communication_handler_impl.h"

#import <memory>
#import <optional>
#import <utility>

#import "base/check.h"
#import "base/functional/bind.h"
#import "base/json/json_reader.h"
#import "base/json/json_writer.h"
#import "base/strings/sys_string_conversions.h"
#import "base/values.h"
#import "ios/web/js_messaging/java_script_content_world.h"
#import "ios/web/js_messaging/java_script_feature_manager.h"
#import "ios/web/public/browser_state.h"
#import "ios/web/public/js_messaging/content_world.h"
#import "ios/web/public/js_messaging/java_script_feature.h"
#import "ios/web/public/js_messaging/script_message.h"
#import "ios/web/public/js_messaging/web_frame.h"
#import "ios/web/public/web_state.h"
#import "net/base/apple/url_conversions.h"
#import "url/gurl.h"

namespace web {

// static
JSPromptCommunicationHandler*
JSPromptCommunicationHandler::GetJSPromptCommunicationHandler(
    WebState* web_state) {
  return JSPromptCommunicationHandlerImpl::FromWebState(web_state);
}

JSPromptCommunicationHandlerImpl::JSPromptCommunicationHandlerImpl(
    WebState* web_state)
    : web_state_(web_state) {
  web_state_->AddObserver(this);
  web_state_->GetPageWorldWebFramesManager()->AddObserver(this);
}

JSPromptCommunicationHandlerImpl::~JSPromptCommunicationHandlerImpl() {
  if (web_state_) {
    web_state_->GetPageWorldWebFramesManager()->RemoveObserver(this);
    web_state_->RemoveObserver(this);
  }
}

bool JSPromptCommunicationHandlerImpl::HandleJavaScriptPrompt(
    GURL request_url,
    BOOL is_main_frame,
    NSString* prompt,
    NSString** result) {
  if ((is_handling_blocked_on_main_frame_ && is_main_frame) || !web_state_) {
    return false;
  }

  std::optional<base::Value> parsed = base::JSONReader::Read(
      base::SysNSStringToUTF8(prompt), base::JSONParserOptions::JSON_PARSE_RFC);
  if (!parsed || !parsed->is_dict()) {
    return false;
  }

  const base::DictValue& dict = parsed->GetDict();
  const std::string* handler_name = dict.FindString("handler");
  const base::Value* message = dict.Find("message");
  if (!handler_name || !message) {
    return false;
  }

  JavaScriptFeature* feature = nullptr;
  for (ContentWorld content_world :
       {ContentWorld::kPageContentWorld, ContentWorld::kIsolatedWorld}) {
    JavaScriptContentWorld* world =
        JavaScriptFeatureManager::GetContentWorldForBrowserState(
            content_world, web_state_->GetBrowserState());
    if (!world) {
      continue;
    }
    if (const JavaScriptFeature* found =
            world->GetFeatureReplyingToPrompts(*handler_name)) {
      feature = const_cast<JavaScriptFeature*>(found);
      break;
    }
  }
  if (!feature) {
    return false;
  }

  ScriptMessage script_message(std::make_unique<base::Value>(message->Clone()),
                               /*is_user_interacting=*/false, is_main_frame,
                               std::move(request_url));

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
  CHECK(replied)
      << "JavaScriptFeature replying to window.prompt must reply synchronously";

  *result = base::SysUTF8ToNSString(reply_json);
  return true;
}

void JSPromptCommunicationHandlerImpl::DidStartNavigation(
    WebState* web_state,
    NavigationContext* navigation_context) {
  is_handling_blocked_on_main_frame_ = false;
}

void JSPromptCommunicationHandlerImpl::PageLoaded(
    WebState* web_state,
    PageLoadCompletionStatus load_completion_status) {
  is_handling_blocked_on_main_frame_ = true;
}

void JSPromptCommunicationHandlerImpl::WebFrameBecameAvailable(
    WebFramesManager* web_frames_manager,
    WebFrame* web_frame) {
  // Once a web frame becomes available we'll know that all document start
  // injected scripts will have run any prompts they would have needed and we
  // can ignore prompts until a new navigation occurs
  if (web_frame->IsMainFrame()) {
    is_handling_blocked_on_main_frame_ = true;
  }
}

void JSPromptCommunicationHandlerImpl::WebStateDestroyed(WebState* web_state) {
  web_state->RemoveObserver(this);
  web_state_ = nullptr;
}

}  // namespace web
