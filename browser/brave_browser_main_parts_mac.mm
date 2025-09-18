/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_parts_mac.h"

#include "base/feature_list.h"
#include "brave/browser/mac/keystone_glue.h"
#include "brave/browser/sparkle_buildflags.h"
#include "brave/browser/updater/buildflags.h"
#include "brave/browser/upgrade_when_idle/upgrade_when_idle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/channel.h"

#if BUILDFLAG(ENABLE_SPARKLE)
#import "brave/browser/mac/sparkle_glue.h"
#endif

#if BUILDFLAG(ENABLE_OMAHA4)
#include "brave/browser/updater/features.h"
#endif

namespace brave {
BASE_FEATURE(kUpgradeWhenIdle,
             "UpgradeWhenIdle",
             base::FEATURE_DISABLED_BY_DEFAULT);
}

BraveBrowserMainPartsMac::BraveBrowserMainPartsMac(bool is_integration_test,
                                                   StartupData* startup_data)
    : ChromeBrowserMainPartsMac(is_integration_test, startup_data) {}

BraveBrowserMainPartsMac::~BraveBrowserMainPartsMac() = default;

void BraveBrowserMainPartsMac::PreCreateMainMessageLoop() {
  ChromeBrowserMainPartsMac::PreCreateMainMessageLoop();

  if (base::FeatureList::IsEnabled(brave::kUpgradeWhenIdle)) {
    upgrade_when_idle_ = std::make_unique<brave::UpgradeWhenIdle>(
        // It's OK to pass profile_manager() here because it stays constant
        // until we reset upgrade_when_idle_ in PostMainMessageLoopRun below.
        g_browser_process->profile_manager());
  }

#if BUILDFLAG(ENABLE_SPARKLE)
#if BUILDFLAG(ENABLE_OMAHA4)
  if (brave_updater::ShouldUseOmaha4()) {
    return;
  }
#endif
  // It would be a no-op if updates are disabled.
  [[SparkleGlue sharedSparkleGlue] registerWithSparkle];
#endif  // BUILDFLAG(ENABLE_SPARKLE)
}

void BraveBrowserMainPartsMac::PostProfileInit(Profile* profile,
                                               bool is_initial_profile) {
  ChromeBrowserMainPartsMac::PostProfileInit(profile, is_initial_profile);

  if (!is_initial_profile) {
    return;
  }

  // Activation of Keystone is not automatic but done in response to the
  // counting and reporting of profiles.
  KeystoneGlue* glue = [KeystoneGlue defaultKeystoneGlue];
  if (glue && ![glue isRegisteredAndActive]) {
    // If profile loading has failed, we still need to handle other tasks
    // like marking of the product as active.
    [glue setRegistrationActive];
  }
}

void BraveBrowserMainPartsMac::PostMainMessageLoopRun() {
  // This code runs before g_browser_process->profile_manager() is destroyed by
  // BrowserProcessImpl::StartTearDown() in upstream's PostMainMessageLoopRun.
  // upgrade_when_idle_ has a pointer to that profile manager and is no longer
  // needed. We therefore reset it here to avoid a potential use-after-free.
  upgrade_when_idle_.reset();
  ChromeBrowserMainPartsMac::PostMainMessageLoopRun();
}
