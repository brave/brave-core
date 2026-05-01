// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/farbling_javascript_feature.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/token.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "brave/ios/browser/brave_shields/brave_shields_settings_service_factory.h"
#include "brave/ios/browser/brave_shields/farbling_args.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/web_state.h"
#include "url/gurl.h"

namespace brave_shields {

namespace {
constexpr char kScriptName[] = "farbling";
}  // namespace

FarblingJavaScriptFeature::FarblingJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {web::JavaScriptFeature::FeatureScript::CreateWithFilename(
              kScriptName,
              web::JavaScriptFeature::FeatureScript::InjectionTime::
                  kDocumentStart,
              web::JavaScriptFeature::FeatureScript::TargetFrames::kAllFrames,
              web::JavaScriptFeature::FeatureScript::ReinjectionBehavior::
                  kInjectOncePerWindow,
              base::BindRepeating(&FarblingJavaScriptFeature::GetReplacements,
                                  base::Unretained(this)))}) {}

FarblingJavaScriptFeature::~FarblingJavaScriptFeature() = default;

// static
FarblingJavaScriptFeature* FarblingJavaScriptFeature::GetInstance() {
  static base::NoDestructor<FarblingJavaScriptFeature> instance;
  return instance.get();
}

web::JavaScriptFeature::FeatureScript::PlaceholderReplacements
FarblingJavaScriptFeature::GetReplacements() {
  NSMutableDictionary* replacements = [[NSMutableDictionary alloc] init];
  [replacements addEntriesFromDictionary:token_.GetPlaceholderReplacements()];
  [replacements
      addEntriesFromDictionary:handler_name_.GetPlaceholderReplacements()];
  return [replacements copy];
}

std::optional<std::string>
FarblingJavaScriptFeature::GetScriptMessageHandlerName() const {
  return handler_name_.GetScriptMessageHandlerName();
}

bool FarblingJavaScriptFeature::GetFeatureRepliesToPrompts() const {
  return true;
}

void FarblingJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  GURL security_origin_url = message.security_origin().GetURL();

  if (!security_origin_url.is_valid() ||
      !security_origin_url.SchemeIsHTTPOrHTTPS() ||
      !token_.GetValidatedScriptMessageBody(message)) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(web_state->GetBrowserState());
  BraveShieldsSettingsService* service =
      BraveShieldsSettingsServiceFactory::GetForProfile(profile);
  bool is_farbling_enabled = service->GetFingerprintMode(security_origin_url) !=
                             mojom::FingerprintMode::ALLOW_MODE;

  base::DictValue value;
  value.Set("isFarblingEnabled", is_farbling_enabled);
  if (is_farbling_enabled) {
    // Seed the farbling args from the origin's persistent farbling token, the
    // same seed source used by the rest of Brave's farbling code.
    HostContentSettingsMap* map =
        ios::HostContentSettingsMapFactory::GetForProfile(profile);
    const base::Token farbling_token = brave_shields::GetFarblingToken(
        map, security_origin_url, /*additional_entropy=*/{});
    value.Set("args", MakeFarblingArgs(farbling_token));
  } else {
    value.Set("args", base::Value());
  }
  base::Value result(std::move(value));
  std::move(callback).Run(&result, nil);
}

}  // namespace brave_shields
