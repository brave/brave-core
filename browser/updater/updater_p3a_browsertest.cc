/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/updater/updater_p3a.h"

#include "base/test/metrics/histogram_tester.h"
#include "chrome/browser/browser_process.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"

namespace brave_updater {

class BraveUpdaterP3ABrowsertest : public InProcessBrowserTest {
 protected:
  void ExpectBucketCount(UpdateStatus status, int count);
  base::HistogramTester histogram_tester_;
};

IN_PROC_BROWSER_TEST_F(BraveUpdaterP3ABrowsertest, PRE_Update) {
  using enum UpdateStatus;
  ExpectBucketCount(kNoUpdateWithLegacy, 0);
  ExpectBucketCount(kNoUpdateWithOmaha4, 0);
  ExpectBucketCount(kUpdatedWithLegacy, 0);
  ExpectBucketCount(kUpdatedWithOmaha4, 0);

  SetLastLaunchVersionForTesting("0.0.0.0", g_browser_process->local_state());
}

IN_PROC_BROWSER_TEST_F(BraveUpdaterP3ABrowsertest, Update) {
  ExpectBucketCount(UpdateStatus::kUpdatedWithLegacy, 1);
}

void BraveUpdaterP3ABrowsertest::ExpectBucketCount(UpdateStatus status,
                                                   int count) {
  histogram_tester_.ExpectBucketCount(kUpdateStatusHistogramName, status,
                                      count);
}

}  // namespace brave_updater
