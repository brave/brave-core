// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/fullscreen/fullscreen_helper_javascript_feature.h"

namespace {
constexpr char kScriptName[] = "fullscreen_helper";
}  // namespace

FullscreenHelperJavaScriptFeature::FullscreenHelperJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentStart,
              FeatureScript::TargetFrames::kAllFrames,
              FeatureScript::ReinjectionBehavior::kInjectOncePerWindow)}) {}

FullscreenHelperJavaScriptFeature::~FullscreenHelperJavaScriptFeature() =
    default;

// static
FullscreenHelperJavaScriptFeature*
FullscreenHelperJavaScriptFeature::GetInstance() {
  static base::NoDestructor<FullscreenHelperJavaScriptFeature> instance;
  return instance.get();
}
