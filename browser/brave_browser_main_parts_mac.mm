/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_parts_mac.h"

#include "base/feature_list.h"
#include "brave/browser/mac/keystone_glue.h"
#include "brave/browser/sparkle_buildflags.h"
#include "brave/browser/upgrade_when_idle/upgrade_when_idle.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/channel.h"

#if BUILDFLAG(ENABLE_SPARKLE)
#import "brave/browser/mac/sparkle_glue.h"
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

#if BUILDFLAG(ENABLE_SPARKLE)
  // It would be no-op if udpate is disabled.
  [[SparkleGlue sharedSparkleGlue] registerWithSparkle];
#endif

  if (base::FeatureList::IsEnabled(brave::kUpgradeWhenIdle)) {
    upgrade_when_idle_ = std::make_unique<brave::UpgradeWhenIdle>();
  }
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
