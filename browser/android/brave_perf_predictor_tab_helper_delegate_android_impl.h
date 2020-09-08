/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_BRAVE_PERF_PREDICTOR_TAB_HELPER_DELEGATE_ANDROID_IMPL_H_
#define BRAVE_BROWSER_ANDROID_BRAVE_PERF_PREDICTOR_TAB_HELPER_DELEGATE_ANDROID_IMPL_H_

#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper_delegate_android.h"

namespace brave {
namespace android {

class BravePerfPredictorTabHelperDelegateAndroidImpl
    : public brave_perf_predictor::PerfPredictorTabHelperDelegateAndroid {
 public:
  BravePerfPredictorTabHelperDelegateAndroidImpl() = default;
  ~BravePerfPredictorTabHelperDelegateAndroidImpl() override {}

  void DispatchSavedBandwidth(uint64_t savings) override;
};

}  // namespace android
}  // namespace brave

#endif  // BRAVE_BROWSER_ANDROID_BRAVE_PERF_PREDICTOR_TAB_HELPER_DELEGATE_ANDROID_IMPL_H_
