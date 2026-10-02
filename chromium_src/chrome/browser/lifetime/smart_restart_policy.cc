/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/lifetime/smart_restart_policy.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)

#include "brave/browser/mac/keystone_glue.h"
#include "brave/browser/updater/buildflags.h"

#if BUILDFLAG(ENABLE_OMAHA4)
#include "brave/browser/updater/features.h"
#endif

#define CanZeroWindowRestartProceed CanZeroWindowRestartProceed_ChromiumImpl
#define CanLockScreenRestartProceed CanLockScreenRestartProceed_ChromiumImpl
#include <chrome/browser/lifetime/smart_restart_policy.cc>
#undef CanLockScreenRestartProceed
#undef CanZeroWindowRestartProceed

namespace smart_restart {

namespace {

std::optional<bool> g_sparkle_is_updater_for_testing;

bool IsSparkleTheUpdater() {
  if (g_sparkle_is_updater_for_testing.has_value()) {
    return *g_sparkle_is_updater_for_testing;
  }
#if BUILDFLAG(ENABLE_OMAHA4)
  if (brave_updater::ShouldUseOmaha4()) {
    return false;
  }
#endif
  // Brave's Keystone glue reports whether Sparkle is active.
  return keystone_glue::KeystoneEnabled();
}

}  // namespace

void SetSparkleIsUpdaterForTesting(std::optional<bool> sparkle_is_updater) {
  g_sparkle_is_updater_for_testing = sparkle_is_updater;
}

// static
bool SmartRestartPolicy::CanZeroWindowRestartProceed() {
  if (IsSparkleTheUpdater()) {
    return false;
  }
  return CanZeroWindowRestartProceed_ChromiumImpl();
}

// static
ExtendedExecutionOutcome SmartRestartPolicy::CanLockScreenRestartProceed(
    const ExtendedRestartabilityState& state) {
  if (IsSparkleTheUpdater()) {
    return ExtendedExecutionOutcome::kBlockedByPolicy;
  }
  return CanLockScreenRestartProceed_ChromiumImpl(state);
}

}  // namespace smart_restart

#else  // BUILDFLAG(IS_MAC)

#include <chrome/browser/lifetime/smart_restart_policy.cc>

#endif  // BUILDFLAG(IS_MAC)
