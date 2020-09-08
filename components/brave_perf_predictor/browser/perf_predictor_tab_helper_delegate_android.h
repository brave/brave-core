/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_DELEGATE_ANDROID_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_DELEGATE_ANDROID_H_

#include <stdint.h>

namespace brave_perf_predictor {

class PerfPredictorTabHelperDelegateAndroid {
 public:
  virtual ~PerfPredictorTabHelperDelegateAndroid() = default;
  virtual void DispatchSavedBandwidth(uint64_t savings) = 0;
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_DELEGATE_ANDROID_H_
