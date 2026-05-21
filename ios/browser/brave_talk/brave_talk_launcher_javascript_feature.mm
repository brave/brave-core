// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_talk/brave_talk_launcher_javascript_feature.h"

#include <optional>
#include <string>

#include "base/containers/fixed_flat_set.h"
#include "base/values.h"
#include "brave/ios/browser/brave_talk/brave_talk_tab_helper.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/web_state.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace {

constexpr char kScriptName[] = "brave_talk_launcher";
constexpr char kScriptHandlerName[] = "BraveTalkLauncherMessageHandler";
constexpr char kMessageURLKey[] = "url";
constexpr auto kAllowedUrlHosts = base::MakeFixedFlatSet<std::string_view>({
    "talk.brave.com",
    "talk.bravesoftware.com",
    "talk.brave.software",
});

}  // namespace

BraveTalkLauncherJavaScriptFeature::BraveTalkLauncherJavaScriptFeature()
    : JavaScriptFeature(web::ContentWorld::kIsolatedWorld,
                        {FeatureScript::CreateWithFilename(
                            kScriptName,
                            FeatureScript::InjectionTime::kDocumentStart,
                            FeatureScript::TargetFrames::kMainFrame,
                            FeatureScript::ReinjectionBehavior::
                                kReinjectOnDocumentRecreation)}) {}

BraveTalkLauncherJavaScriptFeature::~BraveTalkLauncherJavaScriptFeature() =
    default;

// static
BraveTalkLauncherJavaScriptFeature*
BraveTalkLauncherJavaScriptFeature::GetInstance() {
  static base::NoDestructor<BraveTalkLauncherJavaScriptFeature> instance;
  return instance.get();
}

std::optional<std::string>
BraveTalkLauncherJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

void BraveTalkLauncherJavaScriptFeature::ScriptMessageReceived(
    web::WebState* web_state,
    const web::ScriptMessage& message) {
  GURL request_url = message.request_url().value_or(GURL());

  if (!message.is_main_frame() || !request_url.is_valid() ||
      !kAllowedUrlHosts.contains(request_url.host())) {
    return;
  }

  const base::DictValue* body =
      message.body() ? message.body()->GetIfDict() : nullptr;
  if (!body) {
    return;
  }

  const std::string* url_string = body->FindString(kMessageURLKey);
  if (!url_string) {
    return;
  }

  const GURL jitsi_url(*url_string);
  if (!jitsi_url.is_valid()) {
    return;
  }

  // The room name is the first path component of jitsi frame url (stripping the
  // the leading '/').
  const std::string_view room =
      jitsi_url.path().empty() ? "" : jitsi_url.path().substr(1);
  if (room.empty()) {
    return;
  }

  std::string jwt;
  if (!net::GetValueForKeyInQuery(jitsi_url, "jwt", &jwt) || jwt.empty()) {
    return;
  }

  BraveTalkTabHelper* tab_helper = BraveTalkTabHelper::FromWebState(web_state);
  if (!tab_helper) {
    return;
  }
  tab_helper->LaunchBraveTalk(room, jwt);
}
