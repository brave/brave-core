// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_search/brave_search_ad_results_javascript_feature.h"

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/values.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"

namespace {
constexpr char kScriptName[] = "brave_search_ad_results";
// The timeout for any JavaScript call in this file.
constexpr base::TimeDelta kJavaScriptExecutionTimeoutInSeconds =
    base::Seconds(5);
}  // namespace

BraveSearchAdResultsJavaScriptFeature::BraveSearchAdResultsJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kIsolatedWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentEnd,
              FeatureScript::TargetFrames::kMainFrame,
              FeatureScript::ReinjectionBehavior::kInjectOncePerWindow)}) {}

BraveSearchAdResultsJavaScriptFeature::
    ~BraveSearchAdResultsJavaScriptFeature() = default;

// static
BraveSearchAdResultsJavaScriptFeature*
BraveSearchAdResultsJavaScriptFeature::GetInstance() {
  static base::NoDestructor<BraveSearchAdResultsJavaScriptFeature> instance;
  return instance.get();
}

void BraveSearchAdResultsJavaScriptFeature::GetCreatives(
    web::WebState* web_state,
    base::OnceCallback<void(const base::Value*)> callback) {
  web::WebFrame* main_frame = GetWebFramesManager(web_state)->GetMainWebFrame();
  if (!main_frame) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), nullptr));
    return;
  }
  CallJavaScriptFunction(main_frame, "braveSearchAdResults.getCreatives",
                         base::ListValue(), std::move(callback),
                         kJavaScriptExecutionTimeoutInSeconds);
}
