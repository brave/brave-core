/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/global_privacy_control/global_privacy_control_utils.h"

#include "brave/components/global_privacy_control/pref_names.h"
#include "components/prefs/pref_service.h"

namespace global_privacy_control {

bool IsGlobalPrivacyControlEnabled(PrefService* prefs) {
  CHECK(prefs);

  // If the policy is not set, we default to enabled.
  if (!prefs->IsManagedPreference(kGlobalPrivacyControlEnabled)) {
    return true;
  }

  // If the policy is set, we use it.
  return prefs->GetBoolean(kGlobalPrivacyControlEnabled);
}

}  // namespace global_privacy_control
