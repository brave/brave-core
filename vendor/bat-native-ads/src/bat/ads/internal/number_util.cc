/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/number_util.h"

#include <cmath>

namespace ads {

namespace {
const double kEpsilon = .0001;
}  // namespace

bool DoubleEquals(
    const double a,
    const double b) {
  // Choice of epsilon for double comparison allows for proper comparison, we
  // want it to be relatively high to avoid false negative comparison results
  return std::abs(a - b) < kEpsilon;
}

}  // namespace ads
