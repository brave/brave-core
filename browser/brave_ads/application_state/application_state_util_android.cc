/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/application_state_util_android.h"

#include "base/android/application_status_listener.h"

namespace brave_ads {

bool IsApplicationInForeground() {
  return base::android::ApplicationStatusListener::GetState() ==
         base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES;
}

}  // namespace brave_ads
