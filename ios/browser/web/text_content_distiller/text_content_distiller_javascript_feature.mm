// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/text_content_distiller/text_content_distiller_javascript_feature.h"

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "base/values.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"

namespace {

constexpr char kScriptName[] = "text_content_distiller";
constexpr base::TimeDelta kJavaScriptExecutionTimeoutInSeconds =
    base::Seconds(5);

}  // namespace

TextContentDistillerJavaScriptFeature::TextContentDistillerJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kIsolatedWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentEnd,
              FeatureScript::TargetFrames::kMainFrame,
              FeatureScript::ReinjectionBehavior::kInjectOncePerWindow)}) {}

TextContentDistillerJavaScriptFeature::
    ~TextContentDistillerJavaScriptFeature() = default;

// static
TextContentDistillerJavaScriptFeature*
TextContentDistillerJavaScriptFeature::GetInstance() {
  static base::NoDestructor<TextContentDistillerJavaScriptFeature> instance;
  return instance.get();
}

void TextContentDistillerJavaScriptFeature::GetTextContent(
    web::WebState* web_state,
    base::OnceCallback<void(std::string)> callback) {
  web::WebFrame* main_frame = GetWebFramesManager(web_state)->GetMainWebFrame();
  if (!main_frame) {
    std::move(callback).Run(std::string());
    return;
  }
  CallJavaScriptFunction(
      main_frame, "textContentDistiller.getTextContent", {},
      base::BindOnce(
          [](base::OnceCallback<void(std::string)> handler,
             const base::Value* value) {
            std::move(handler).Run(value && value->is_string()
                                       ? value->GetString()
                                       : std::string());
          },
          std::move(callback)),
      kJavaScriptExecutionTimeoutInSeconds);
}
