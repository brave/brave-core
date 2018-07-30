/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_parts_mac.h"

#import "brave/browser/mac/sparkle_glue.h"

void BraveBrowserMainPartsMac::PreMainMessageLoopStart() {
  ChromeBrowserMainPartsMac::PreMainMessageLoopStart();

  // It would be no-op if udpate is disabled.
  [[SparkleGlue sharedSparkleGlue] registerWithSparkle];
}
