// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/js_messaging/safe_builtins_javascript_feature.h"

#include "ios/web/public/js_messaging/java_script_feature.h"

namespace {
constexpr char kScriptName[] = "safe_builtins";
}

SafeBuiltinsJavaScriptFeature::SafeBuiltinsJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kAllContentWorlds,
          std::vector<JavaScriptFeature::FeatureScript>(
              {JavaScriptFeature::FeatureScript::CreateWithFilename(
                  kScriptName,
                  JavaScriptFeature::FeatureScript::InjectionTime::
                      kDocumentStart,
                  JavaScriptFeature::FeatureScript::TargetFrames::
                      kAllFrames)})) {}

SafeBuiltinsJavaScriptFeature::~SafeBuiltinsJavaScriptFeature() = default;

// static
SafeBuiltinsJavaScriptFeature* SafeBuiltinsJavaScriptFeature::GetInstance() {
  static base::NoDestructor<SafeBuiltinsJavaScriptFeature> instance;
  return instance.get();
}
