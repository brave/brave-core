// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/page_metadata/page_metadata_javascript_feature.h"

#include "base/time/time.h"
#include "base/values.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"

namespace {
constexpr char kScriptName[] = "page_metadata";
// The timeout for any JavaScript call in this file.
constexpr int64_t kJavaScriptExecutionTimeoutInSeconds = 5;
}  // namespace

PageMetadataJavaScriptFeature::PageMetadataJavaScriptFeature()
    : JavaScriptFeature(web::ContentWorld::kIsolatedWorld,
                        {FeatureScript::CreateWithFilename(
                            kScriptName,
                            FeatureScript::InjectionTime::kDocumentEnd,
                            FeatureScript::TargetFrames::kMainFrame,
                            FeatureScript::ReinjectionBehavior::
                                kReinjectOnDocumentRecreation)}) {}

PageMetadataJavaScriptFeature::~PageMetadataJavaScriptFeature() = default;

// static
PageMetadataJavaScriptFeature* PageMetadataJavaScriptFeature::GetInstance() {
  static base::NoDestructor<PageMetadataJavaScriptFeature> instance;
  return instance.get();
}

void PageMetadataJavaScriptFeature::GetMetadata(
    web::WebState* web_state,
    base::OnceCallback<void(const base::Value*)> callback) {
  web::WebFrame* main_frame = GetWebFramesManager(web_state)->GetMainWebFrame();
  if (!main_frame) {
    std::move(callback).Run(nullptr);
    return;
  }
  CallJavaScriptFunction(main_frame, "pageMetadata.getMetadata",
                         base::ListValue(), std::move(callback),
                         base::Seconds(kJavaScriptExecutionTimeoutInSeconds));
}
