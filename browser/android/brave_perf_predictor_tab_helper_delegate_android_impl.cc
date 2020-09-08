/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_perf_predictor_tab_helper_delegate_android_impl.h"

#include "brave/browser/android/brave_shields_content_settings.h"

namespace brave {
namespace android {

void BravePerfPredictorTabHelperDelegateAndroidImpl::DispatchSavedBandwidth(
    uint64_t savings) {
  chrome::android::BraveShieldsContentSettings::DispatchSavedBandwidth(
          savings);
}

}  // namespace android
}  // namespace brave
