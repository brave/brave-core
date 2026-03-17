// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/reader_mode/reader_mode_javascript_feature.h"

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "base/values.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"

namespace {
constexpr char kReadabilityScriptName[] = "Readability";
constexpr char kReadabilityReaderableScriptName[] = "Readability-readerable";
constexpr char kScriptName[] = "brave_reader_mode";
// The timeout for any JavaScript call in this file.
constexpr base::TimeDelta kJavaScriptExecutionTimeoutInSeconds =
    base::Seconds(5);
}  // namespace

namespace brave {

ReaderModeJavaScriptFeature::ReaderModeJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kIsolatedWorld,
          {FeatureScript::CreateWithFilename(
               kReadabilityScriptName,
               FeatureScript::InjectionTime::kDocumentStart,
               FeatureScript::TargetFrames::kMainFrame,
               FeatureScript::ReinjectionBehavior::
                   kReinjectOnDocumentRecreation),
           FeatureScript::CreateWithFilename(
               kReadabilityReaderableScriptName,
               FeatureScript::InjectionTime::kDocumentStart,
               FeatureScript::TargetFrames::kMainFrame,
               FeatureScript::ReinjectionBehavior::
                   kReinjectOnDocumentRecreation),
           FeatureScript::CreateWithFilename(
               kScriptName,
               FeatureScript::InjectionTime::kDocumentStart,
               FeatureScript::TargetFrames::kMainFrame,
               FeatureScript::ReinjectionBehavior::kInjectOncePerWindow)}) {}

ReaderModeJavaScriptFeature::~ReaderModeJavaScriptFeature() = default;

// static
ReaderModeJavaScriptFeature* ReaderModeJavaScriptFeature::GetInstance() {
  static base::NoDestructor<ReaderModeJavaScriptFeature> instance;
  return instance.get();
}

void ReaderModeJavaScriptFeature::CheckReadability(
    web::WebState* web_state,
    base::OnceCallback<void(const std::string&)> callback) {
  web::WebFrame* main_frame = GetWebFramesManager(web_state)->GetMainWebFrame();
  if (!main_frame) {
    std::move(callback).Run("");
    return;
  }
  auto internal_callback = base::BindOnce(
      [](base::OnceCallback<void(const std::string&)> callback,
         const base::Value* value) {
        std::move(callback).Run(value && value->is_string() ? value->GetString()
                                                            : "");
      },
      std::move(callback));
  CallJavaScriptFunction(main_frame, "readerMode.checkReadability",
                         base::ListValue(), std::move(internal_callback),
                         kJavaScriptExecutionTimeoutInSeconds);
}

void ReaderModeJavaScriptFeature::SetStyle(web::WebState* web_state,
                                           const base::DictValue& style_dict) {
  web::WebFrame* main_frame = GetWebFramesManager(web_state)->GetMainWebFrame();
  if (!main_frame) {
    return;
  }
  auto parameters = base::ListValue().Append(style_dict.Clone());
  CallJavaScriptFunction(main_frame, "readerMode.setStyle", parameters);
}

}  // namespace brave
