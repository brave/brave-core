/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/global_privacy_control/global_privacy_control.h"

#include "base/feature_list.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace blink {

namespace {

bool IsShieldsAdBlockOnlyModeEnabled(ExecutionContext* context) {
  if (!context) {
    return false;
  }
  LocalFrame* local_frame = To<LocalDOMWindow>(context)->GetFrame();
  if (!local_frame || !local_frame->GetContentSettingsClient()) {
    return false;
  }
  return local_frame->GetContentSettingsClient()
      ->IsShieldsAdBlockOnlyModeEnabled();
}

}  // namespace

bool GlobalPrivacyControl::globalPrivacyControl(NavigatorBase& navigator) {
  return base::FeatureList::IsEnabled(features::kBraveGlobalPrivacyControl) &&
         !IsShieldsAdBlockOnlyModeEnabled(navigator.GetExecutionContext());
}

}  // namespace blink
