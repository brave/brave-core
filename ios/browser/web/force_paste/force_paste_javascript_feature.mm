// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/force_paste/force_paste_javascript_feature.h"

#include "ios/web/public/js_messaging/script_message.h"
#include "ios/web/public/js_messaging/web_frame.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state.h"

namespace {
const char kScriptName[] = "force_paste";
}  // namespace

ForcePasteJavaScriptFeature::ForcePasteJavaScriptFeature()
    : JavaScriptFeature(
          web::ContentWorld::kIsolatedWorld,
          {FeatureScript::CreateWithFilename(
              kScriptName,
              FeatureScript::InjectionTime::kDocumentEnd,
              FeatureScript::TargetFrames::kMainFrame,
              FeatureScript::ReinjectionBehavior::kInjectOncePerWindow)}) {}

ForcePasteJavaScriptFeature::~ForcePasteJavaScriptFeature() = default;

// static
ForcePasteJavaScriptFeature* ForcePasteJavaScriptFeature::GetInstance() {
  static base::NoDestructor<ForcePasteJavaScriptFeature> instance;
  return instance.get();
}

void ForcePasteJavaScriptFeature::ForcePaste(web::WebState* web_state,
                                             std::string contents) {
  web::WebFrame* main_frame = GetWebFramesManager(web_state)->GetMainWebFrame();
  if (!main_frame) {
    return;
  }
  auto parameters = base::ListValue().Append(contents);
  CallJavaScriptFunction(main_frame, "forcePaste.pasteIntoActiveElement",
                         parameters);
}
