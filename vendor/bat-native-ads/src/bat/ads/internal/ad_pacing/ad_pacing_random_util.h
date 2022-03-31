/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_RANDOM_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_RANDOM_UTIL_H_

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

double GenerateAdPacingRandomNumber();

class ScopedAdPacingRandomNumberSetter final {
 public:
  explicit ScopedAdPacingRandomNumberSetter(double number);
  ~ScopedAdPacingRandomNumberSetter();
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_PACING_AD_PACING_RANDOM_UTIL_H_
