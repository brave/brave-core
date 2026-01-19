/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_tab_helper.h"

#include "chrome/test/base/platform_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace metrics {

class SerpMetricsTabHelperTest : public PlatformBrowserTest {};

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordBraveSearchEngineResultPage) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordGoogleSearchEngineResultPage) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       RecordOtherSearchEngineResultPage) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordIfUsagePingDisabled) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordIfNavigationWasRestored) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordWithoutUserGesture) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest, DoNotRecordForBackNavigation) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordForForwardNavigation) {
  FAIL();
}

IN_PROC_BROWSER_TEST_F(SerpMetricsTabHelperTest,
                       DoNotRecordForReloadNavigation) {
  FAIL();
}

}  // namespace metrics
