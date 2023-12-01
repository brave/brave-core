/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PACING_PACING_RANDOM_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PACING_PACING_RANDOM_UTIL_H_

namespace brave_ads {

double GeneratePacingRandomNumber();

class ScopedPacingRandomNumberSetterForTesting final {
 public:
  explicit ScopedPacingRandomNumberSetterForTesting(double number);

  ~ScopedPacingRandomNumberSetterForTesting();
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_ELIGIBLE_ADS_PACING_PACING_RANDOM_UTIL_H_
