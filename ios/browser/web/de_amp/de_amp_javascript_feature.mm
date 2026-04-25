// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/de_amp/de_amp_javascript_feature.h"

#include <optional>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/values.h"
#include "brave/components/de_amp/common/pref_names.h"
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

constexpr char kScriptName[] = "de_amp";
constexpr char kScriptHandlerName[] = "DeAmpMessageHandler";
constexpr char kMessageDestinationURLKey[] = "destURL";

bool ShouldRedirect(web::WebState* web_state, const GURL& dest_url) {
  auto* navigation_manager = web_state->GetNavigationManager();
  if (!navigation_manager) {
    return false;
  }

  // Mirror the Swift logic: allow redirect only if dest_url differs from the
  // previous committed URL and the last two committed items are distinct,
  // preventing both client-side and server-side redirect loops.
  const int last_index = navigation_manager->GetLastCommittedItemIndex();
  web::NavigationItem* last_item = navigation_manager->GetLastCommittedItem();
  web::NavigationItem* previous_item =
      last_index > 0 ? navigation_manager->GetItemAtIndex(last_index - 1)
                     : nullptr;

  const GURL last_committed_url = last_item ? last_item->GetURL() : GURL();
  const GURL previous_committed_url =
      previous_item ? previous_item->GetURL() : GURL();

  return dest_url != previous_committed_url &&
         last_committed_url != previous_committed_url;
}

}  // namespace

DeAmpJavaScriptFeature::DeAmpJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kIsolatedWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentStart,
              FeatureScript::TargetFrames::kMainFrame,
              FeatureScript::ReinjectionBehavior::kInjectOncePerWindow)}) {}

DeAmpJavaScriptFeature::~DeAmpJavaScriptFeature() = default;

// static
DeAmpJavaScriptFeature* DeAmpJavaScriptFeature::GetInstance() {
  static base::NoDestructor<DeAmpJavaScriptFeature> instance;
  return instance.get();
}

std::optional<std::string> DeAmpJavaScriptFeature::GetScriptMessageHandlerName()
    const {
  return kScriptHandlerName;
}

bool DeAmpJavaScriptFeature::GetFeatureRepliesToMessages() const {
  return true;
}

void DeAmpJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  auto reply_handler = base::BindOnce(
      [](ScriptMessageReplyCallback handler, bool should_redirect) {
        base::Value value(should_redirect);
        std::move(handler).Run(&value, nil);
      },
      std::move(callback));

  if (!ProfileIOS::FromBrowserState(web_state->GetBrowserState())
           ->GetPrefs()
           ->GetBoolean(de_amp::kDeAmpPrefEnabled)) {
    std::move(reply_handler).Run(false);
    return;
  }

  const base::DictValue* script_dict =
      message.body() ? message.body()->GetIfDict() : nullptr;
  if (!script_dict) {
    std::move(reply_handler).Run(false);
    return;
  }

  const std::string* dest_url_string =
      script_dict->FindString(kMessageDestinationURLKey);
  if (!dest_url_string) {
    std::move(reply_handler).Run(false);
    return;
  }

  const GURL dest_url(*dest_url_string);
  if (!dest_url.is_valid()) {
    std::move(reply_handler).Run(false);
    return;
  }

  bool should_redirect = ShouldRedirect(web_state, dest_url);
  std::move(reply_handler).Run(should_redirect);
}
