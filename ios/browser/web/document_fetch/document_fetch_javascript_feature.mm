// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/document_fetch/document_fetch_javascript_feature.h"

#include <optional>
#include <string>

#include "base/values.h"
#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"
#include "url/gurl.h"

namespace {

constexpr char kScriptName[] = "document_fetch";
constexpr char kScriptHandlerName[] = "DocumentFetchMessageHandler";
constexpr char kStatusCodeKey[] = "statusCode";
constexpr char kBase64DataKey[] = "base64Data";

}  // namespace

DocumentFetchJavaScriptFeature::DocumentFetchJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kIsolatedWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentEnd,
              FeatureScript::TargetFrames::kMainFrame,
              FeatureScript::ReinjectionBehavior::kInjectOncePerWindow)}) {}

DocumentFetchJavaScriptFeature::~DocumentFetchJavaScriptFeature() = default;

// static
DocumentFetchJavaScriptFeature* DocumentFetchJavaScriptFeature::GetInstance() {
  static base::NoDestructor<DocumentFetchJavaScriptFeature> instance;
  return instance.get();
}

void DocumentFetchJavaScriptFeature::DownloadDocument(
    web::WebState* web_state,
    const GURL& url,
    DownloadCallback callback) {
  web::WebFrame* main_frame = GetWebFramesManager(web_state)->GetMainWebFrame();
  if (!main_frame) {
    std::move(callback).Run(0, "");
    return;
  }

  pending_callbacks_[web_state->GetUniqueIdentifier()] = std::move(callback);

  auto parameters = base::ListValue().Append(url.spec());
  CallJavaScriptFunction(main_frame, "documentFetch.download", parameters);
}

std::optional<std::string>
DocumentFetchJavaScriptFeature::GetScriptMessageHandlerName() const {
  return kScriptHandlerName;
}

void DocumentFetchJavaScriptFeature::ScriptMessageReceived(
    web::WebState* web_state,
    const web::ScriptMessage& message) {
  auto it = pending_callbacks_.find(web_state->GetUniqueIdentifier());
  if (it == pending_callbacks_.end()) {
    return;
  }
  auto callback = std::move(it->second);
  pending_callbacks_.erase(it);

  const base::DictValue* dict =
      message.body() ? message.body()->GetIfDict() : nullptr;
  if (!dict) {
    std::move(callback).Run(0, "");
    return;
  }

  // All numbers from the WKWebView JS bridge arrive as doubles regardless of
  // their type on the JS side, so read as double and cast to int.
  std::optional<double> status_code = dict->FindDouble(kStatusCodeKey);
  const std::string* base64_data = dict->FindString(kBase64DataKey);
  std::move(callback).Run(status_code ? static_cast<int>(*status_code) : 0,
                          base64_data ? *base64_data : "");
}
