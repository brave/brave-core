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
#include "base/task/task_traits.h"
#include "brave/browser/updater/features.h"
#include "brave/updater/updater_p3a.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_constants.h"
#include "content/public/browser/browser_thread.h"
#endif

void BraveBrowserMainPartsMac::PreCreateMainMessageLoop() {
  ChromeBrowserMainPartsMac::PreCreateMainMessageLoop();

#if BUILDFLAG(ENABLE_SPARKLE)
  // It would be no-op if udpate is disabled.
  [[SparkleGlue sharedSparkleGlue] registerWithSparkle];
#endif
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

void BraveBrowserMainPartsMac::PostBrowserStart() {
  ChromeBrowserMainPartsMac::PostBrowserStart();
#if BUILDFLAG(ENABLE_OMAHA4)
  content::GetUIThreadTaskRunner({base::TaskPriority::BEST_EFFORT})
      ->PostTask(FROM_HERE, base::BindOnce([]() {
                   brave_updater::ReportLaunch(
                       base::Time::Now(), chrome::kChromeVersion,
                       brave_updater::ShouldUseOmaha4(),
                       g_browser_process->local_state());
                 }));
#endif  // BUILDFLAG(ENABLE_OMAHA4)
}
