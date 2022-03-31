/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_pacing/ad_pacing_random_util.h"

#include "base/rand_util.h"

namespace {

absl::optional<double> g_ad_pacing_random_number;

}  // namespace

namespace ads {

double GenerateAdPacingRandomNumber() {
  if (g_ad_pacing_random_number) {
    return *g_ad_pacing_random_number;
  }

  return base::RandDouble();
}

ScopedAdPacingRandomNumberSetter::ScopedAdPacingRandomNumberSetter(
    double number) {
  g_ad_pacing_random_number = number;
}

ScopedAdPacingRandomNumberSetter::~ScopedAdPacingRandomNumberSetter() {
  g_ad_pacing_random_number = absl::nullopt;
}

}  // namespace ads
