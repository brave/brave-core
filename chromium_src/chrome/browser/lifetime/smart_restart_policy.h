/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_LIFETIME_SMART_RESTART_POLICY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_LIFETIME_SMART_RESTART_POLICY_H_

#include <optional>

#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
#define CanZeroWindowRestartProceed           \
  CanZeroWindowRestartProceed_ChromiumImpl(); \
  static bool CanZeroWindowRestartProceed

#define CanLockScreenRestartProceed              \
  CanLockScreenRestartProceed_ChromiumImpl(      \
      const ExtendedRestartabilityState& state); \
  static ExtendedExecutionOutcome CanLockScreenRestartProceed
#endif  // BUILDFLAG(IS_MAC)

#include <chrome/browser/lifetime/smart_restart_policy.h>  // IWYU pragma: export

#if BUILDFLAG(IS_MAC)
#undef CanLockScreenRestartProceed
#undef CanZeroWindowRestartProceed

namespace smart_restart {

// Sparkle replaces the whole app bundle when it installs an update on quit, so
// a SmartRestart relaunch can start the old version while its Versions folder
// is being removed. SmartRestart therefore never restarts while Sparkle is the
// updater. Tests use this to set whether Sparkle is the updater; std::nullopt
// restores the real check.
void SetSparkleIsUpdaterForTesting(std::optional<bool> sparkle_is_updater);

}  // namespace smart_restart
#endif  // BUILDFLAG(IS_MAC)

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_LIFETIME_SMART_RESTART_POLICY_H_
