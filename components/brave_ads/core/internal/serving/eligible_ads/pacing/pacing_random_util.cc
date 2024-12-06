/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/pacing/pacing_random_util.h"

#include <optional>

#include "base/check_is_test.h"
#include "base/rand_util.h"

namespace {
std::optional<double> g_pacing_random_number_for_testing;
}  // namespace

namespace brave_ads {

double GeneratePacingRandomNumber() {
  if (g_pacing_random_number_for_testing) {
    CHECK_IS_TEST();

    return *g_pacing_random_number_for_testing;
  }

  return base::RandDouble();
}

ScopedPacingRandomNumberSetterForTesting::
    ScopedPacingRandomNumberSetterForTesting(double number) {
  CHECK_IS_TEST();

  g_pacing_random_number_for_testing = number;
}

ScopedPacingRandomNumberSetterForTesting::
    ~ScopedPacingRandomNumberSetterForTesting() {
  g_pacing_random_number_for_testing = std::nullopt;
}

}  // namespace brave_ads
