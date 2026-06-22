// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/protection_stats_javascript_feature.h"

#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "brave/ios/browser/brave_shields/protection_stats_tab_helper.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/web_state.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_shields {

namespace {
constexpr char kScriptName[] = "protection_stats";
constexpr char kScriptHandlerName[] = "ProtectionStatsMessageHandler";
constexpr char kResourceURLKey[] = "resourceURL";
constexpr char kResourceTypeKey[] = "resourceType";
}  // namespace

ProtectionStatsJavaScriptFeature::ProtectionStatsJavaScriptFeature()
    : JavaScriptFeature(web::ContentWorld::kPageContentWorld,
                        {FeatureScript::CreateWithFilename(
                            kScriptName,
                            FeatureScript::InjectionTime::kDocumentStart,
                            FeatureScript::TargetFrames::kAllFrames,
                            FeatureScript::ReinjectionBehavior::
                                kReinjectOnDocumentRecreation)}) {}

ProtectionStatsJavaScriptFeature::~ProtectionStatsJavaScriptFeature() = default;

// static
ProtectionStatsJavaScriptFeature*
ProtectionStatsJavaScriptFeature::GetInstance() {
  static base::NoDestructor<ProtectionStatsJavaScriptFeature> instance;
  return instance.get();
}

std::optional<std::string>
ProtectionStatsJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

void ProtectionStatsJavaScriptFeature::ScriptMessageReceived(
    web::WebState* web_state,
    const web::ScriptMessage& message) {
  const base::ListValue* body =
      message.body() ? message.body()->GetIfList() : nullptr;
  if (!body) {
    return;
  }

  auto* tab_helper = ProtectionStatsTabHelper::FromWebState(web_state);
  if (!tab_helper) {
    return;
  }

  std::vector<BlockedResource> resources;
  for (const base::Value& value : *body) {
    const base::DictValue* resource = value.GetIfDict();
    if (!resource) {
      continue;
    }
    const std::string* resource_url = resource->FindString(kResourceURLKey);
    const std::string* resource_type = resource->FindString(kResourceTypeKey);
    if (!resource_url || !resource_type) {
      continue;
    }
    resources.push_back({*resource_url, *resource_type});
  }

  if (resources.empty()) {
    return;
  }

  tab_helper->HandleBlockedResources(resources,
                                     message.security_origin().GetURL());
}

}  // namespace brave_shields
