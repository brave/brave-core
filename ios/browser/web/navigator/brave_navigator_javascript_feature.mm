// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/navigator/brave_navigator_javascript_feature.h"

namespace {
constexpr char kScriptName[] = "brave_navigator";
}  // namespace

BraveNavigatorJavaScriptFeature::BraveNavigatorJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kPageContentWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentStart,
              FeatureScript::TargetFrames::kAllFrames,
              FeatureScript::ReinjectionBehavior::kInjectOncePerWindow)}) {}

BraveNavigatorJavaScriptFeature::~BraveNavigatorJavaScriptFeature() = default;

// static
BraveNavigatorJavaScriptFeature*
BraveNavigatorJavaScriptFeature::GetInstance() {
  static base::NoDestructor<BraveNavigatorJavaScriptFeature> instance;
  return instance.get();
}
