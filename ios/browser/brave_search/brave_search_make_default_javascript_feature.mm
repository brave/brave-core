// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_search/brave_search_make_default_javascript_feature.h"

#include <optional>
#include <string>

#include "base/containers/fixed_flat_set.h"
#include "base/numerics/safe_conversions.h"
#include "base/values.h"
#include "brave/ios/browser/brave_search/brave_search_make_default_tab_helper.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/web_state.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

constexpr char kScriptName[] = "brave_search_make_default_helper";
constexpr char kScriptHandlerName[] = "BraveSearchMakeDefaultMessageHandler";
constexpr char kMethodIdKey[] = "method_id";
constexpr int kMethodGetCanSetDefault = 1;
constexpr int kMethodSetDefault = 2;
constexpr auto kAllowedHosts = base::MakeFixedFlatSet<std::string_view>({
    "safesearch.brave.com",
    "safesearch.brave.software",
    "safesearch.bravesoftware.com",
    "search-dev-local.brave.com",
    "search.brave.com",
    "search.brave.software",
    "search.bravesoftware.com",
});

}  // namespace

BraveSearchMakeDefaultJavaScriptFeature::
    BraveSearchMakeDefaultJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentStart,
              FeatureScript::TargetFrames::kMainFrame,
              FeatureScript::ReinjectionBehavior::kInjectOncePerWindow)}) {}

BraveSearchMakeDefaultJavaScriptFeature::
    ~BraveSearchMakeDefaultJavaScriptFeature() = default;

// static
BraveSearchMakeDefaultJavaScriptFeature*
BraveSearchMakeDefaultJavaScriptFeature::GetInstance() {
  static base::NoDestructor<BraveSearchMakeDefaultJavaScriptFeature> instance;
  return instance.get();
}

bool BraveSearchMakeDefaultJavaScriptFeature::GetFeatureRepliesToMessages()
    const {
  return true;
}

std::optional<std::string>
BraveSearchMakeDefaultJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

void BraveSearchMakeDefaultJavaScriptFeature::ScriptMessageReceivedWithReply(
    web::WebState* web_state,
    const web::ScriptMessage& message,
    ScriptMessageReplyCallback callback) {
  GURL request_url = message.request_url().value_or(GURL());

  if (!message.is_main_frame() || !request_url.is_valid() ||
      !request_url.SchemeIs(url::kHttpsScheme) ||
      !kAllowedHosts.contains(request_url.host())) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  const base::DictValue* body =
      message.body() ? message.body()->GetIfDict() : nullptr;
  if (!body) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  // All numbers from the WKWebView JS bridge arrive as doubles regardless of
  // their type on the JS side, so read as double and cast to int.
  std::optional<double> method_id = body->FindDouble(kMethodIdKey);
  if (!method_id) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  BraveSearchMakeDefaultTabHelper* tab_helper =
      BraveSearchMakeDefaultTabHelper::FromWebState(web_state);
  if (!tab_helper) {
    std::move(callback).Run(nullptr, nil);
    return;
  }

  switch (base::saturated_cast<int>(*method_id)) {
    case kMethodGetCanSetDefault: {
      const base::Value reply(tab_helper->GetCanSetDefaultSearchProvider());
      std::move(callback).Run(&reply, nil);
      return;
    }
    case kMethodSetDefault:
      tab_helper->SetIsDefaultSearchProvider();
      std::move(callback).Run(nullptr, nil);
      return;
    default:
      std::move(callback).Run(nullptr, nil);
  }
}
