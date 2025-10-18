/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/global_privacy_control/global_privacy_control.h"

#include "base/feature_list.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/workers/worker_or_worklet_global_scope.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace blink {

namespace {

// Dynamic iframes without a committed navigation don't have content settings
// rules filled, so we always look for the root frame which has required data
// for shields/farbling to be enabled.
blink::WebContentSettingsClient* GetContentSettingsFromFrame(
    blink::LocalFrame* local_frame) {
  if (!local_frame) {
    return nullptr;
  }

  blink::WebContentSettingsClient* content_settings =
      local_frame->LocalFrameRoot().GetContentSettingsClient();
  if (!content_settings || !content_settings->HasContentSettingsRules()) {
    return nullptr;
  }
  return content_settings;
}

blink::WebContentSettingsClient* GetContentSettingsClientFromContext(
    ExecutionContext* context) {
  if (!context) {
    return nullptr;
  }

  if (LocalDOMWindow* window = DynamicTo<LocalDOMWindow>(context)) {
    if (WebContentSettingsClient* content_settings =
            GetContentSettingsFromFrame(window->GetDisconnectedFrame())) {
      return content_settings;
    }

    return GetContentSettingsFromFrame(window->GetFrame());
  }

  if (WorkerOrWorkletGlobalScope* scope =
          DynamicTo<WorkerOrWorkletGlobalScope>(context)) {
    return scope->ContentSettingsClient();
  }

  return nullptr;
}

bool IsGlobalPrivacyControlDisabledByPolicy(ExecutionContext* context) {
  if (WebContentSettingsClient* content_settings =
          GetContentSettingsClientFromContext(context)) {
    return content_settings->IsGlobalPrivacyControlDisabledByPolicy();
  }

  return false;
}

}  // namespace

bool GlobalPrivacyControl::globalPrivacyControl(NavigatorBase& navigator) {
  return base::FeatureList::IsEnabled(features::kBraveGlobalPrivacyControl) &&
         !IsGlobalPrivacyControlDisabledByPolicy(
             navigator.GetExecutionContext());
}

}  // namespace blink
