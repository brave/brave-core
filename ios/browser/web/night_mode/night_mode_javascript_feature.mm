// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/night_mode/night_mode_javascript_feature.h"

namespace {
constexpr char kDarkReaderScriptName[] = "darkreader";
}  // namespace

NightModeJavaScriptFeature::NightModeJavaScriptFeature()
    : JavaScriptFeature(web::ContentWorld::kIsolatedWorld,
                        {FeatureScript::CreateWithFilename(
                            kDarkReaderScriptName,
                            FeatureScript::InjectionTime::kDocumentStart,
                            FeatureScript::TargetFrames::kMainFrame,
                            FeatureScript::ReinjectionBehavior::
                                kReinjectOnDocumentRecreation)}) {}

NightModeJavaScriptFeature::~NightModeJavaScriptFeature() = default;

// static
NightModeJavaScriptFeature* NightModeJavaScriptFeature::GetInstance() {
  static base::NoDestructor<NightModeJavaScriptFeature> instance;
  return instance.get();
}
