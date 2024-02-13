/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/privacy_sandbox/tracking_protection_onboarding.h"

#define MaybeMarkEligible MaybeMarkEligible_ChromiumImpl
#include "src/components/privacy_sandbox/tracking_protection_onboarding.cc"
#undef MaybeMarkEligible

namespace privacy_sandbox {

void TrackingProtectionOnboarding::MaybeMarkEligible() {
  DCHECK_EQ(TrackingProtectionOnboardingStatus::kIneligible,
            GetInternalOnboardingStatus(pref_service_));
  return;
}

}  // namespace privacy_sandbox
