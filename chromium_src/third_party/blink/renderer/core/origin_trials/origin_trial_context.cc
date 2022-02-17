/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/origin_trials/origin_trial_context.h"

#define AddFeature AddFeature_ChromiumImpl
#define AddForceEnabledTrials AddForceEnabledTrials_ChromiumImpl
#include "src/third_party/blink/renderer/core/origin_trials/origin_trial_context.cc"
#undef AddForceEnabledTrials
#undef AddFeature

namespace blink {

// AddFeature doesn't check if origin_trials::IsTrialValid.
void OriginTrialContext::AddFeature(OriginTrialFeature feature) {
  if (origin_trials::IsTrialDisabledInBrave(feature))
    return;

  AddFeature_ChromiumImpl(feature);
}

// AddForceEnabledTrials only has a DCHECK with origin_trials::IsTrialValid.
void OriginTrialContext::AddForceEnabledTrials(
    const Vector<String>& trial_names) {
  for (const String& trial_name : trial_names) {
    if (origin_trials::IsTrialDisabledInBrave(trial_name.Utf8()))
      return;
  }

  AddForceEnabledTrials_ChromiumImpl(trial_names);
}

}  // namespace blink
