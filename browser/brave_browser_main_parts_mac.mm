/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_parts_mac.h"

#include "brave/browser/mac/keystone_glue.h"
#include "brave/browser/sparkle_buildflags.h"
#include "brave/browser/updater/buildflags.h"

#if BUILDFLAG(ENABLE_SPARKLE)
#import "brave/browser/mac/sparkle_glue.h"
#endif

#if BUILDFLAG(ENABLE_OMAHA4)
#include "brave/browser/updater/features.h"
#endif

void BraveBrowserMainPartsMac::PreCreateMainMessageLoop() {
  ChromeBrowserMainPartsMac::PreCreateMainMessageLoop();

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
