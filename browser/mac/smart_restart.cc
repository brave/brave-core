/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/mac/smart_restart.h"

#include "brave/browser/mac/keystone_glue.h"
#include "brave/browser/updater/buildflags.h"

#if BUILDFLAG(ENABLE_OMAHA4)
#include "brave/browser/updater/features.h"
#endif  // BUILDFLAG(ENABLE_OMAHA4)

namespace {

std::optional<bool> g_sparkle_is_updater_for_testing;

}  // namespace

bool BraveIsSparkleTheUpdater() {
  if (g_sparkle_is_updater_for_testing.has_value()) {
    return *g_sparkle_is_updater_for_testing;
  }
#if BUILDFLAG(ENABLE_OMAHA4)
  // Check Omaha 4 first: asking the Sparkle glue for its state loads the
  // Sparkle framework, which must not happen while Omaha 4 is in use.
  if (brave_updater::ShouldUseOmaha4()) {
    return false;
  }
#endif  // BUILDFLAG(ENABLE_OMAHA4)
  // Brave's Keystone glue reports whether Sparkle is active.
  return keystone_glue::KeystoneEnabled();
}

void SetSparkleIsUpdaterForTesting(std::optional<bool> sparkle_is_updater) {
  g_sparkle_is_updater_for_testing = sparkle_is_updater;
}
