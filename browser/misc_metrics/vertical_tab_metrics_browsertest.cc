/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/metrics/histogram_tester.h"
#include "brave/browser/misc_metrics/vertical_tab_metrics.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"

namespace misc_metrics {

class VerticalTabMetricsTest : public InProcessBrowserTest {
 protected:
  void SetVerticalTabsEnabled(bool enabled) {
    return browser()->profile()->GetPrefs()->SetBoolean(
        brave_tabs::kVerticalTabsEnabled, enabled);
  }

  base::HistogramTester histogram_tester_;
};

IN_PROC_BROWSER_TEST_F(VerticalTabMetricsTest, OpenTabs) {
  chrome::AddTabAt(browser(), {}, -1, true);

  histogram_tester_.ExpectTotalCount(kVerticalOpenTabsHistogramName, 0);

  SetVerticalTabsEnabled(true);

  chrome::AddTabAt(browser(), {}, -1, true);
  histogram_tester_.ExpectUniqueSample(kVerticalOpenTabsHistogramName, 1, 1);

  for (size_t i = 0; i < 2; i++) {
    chrome::AddTabAt(browser(), {}, -1, true);
  }
  histogram_tester_.ExpectUniqueSample(kVerticalOpenTabsHistogramName, 1, 3);

  chrome::AddTabAt(browser(), {}, -1, true);
  histogram_tester_.ExpectBucketCount(kVerticalOpenTabsHistogramName, 2, 1);

  // Test combined counts between two windows
  Browser* second_browser = CreateBrowser(browser()->profile());
  for (size_t i = 0; i < 4; i++) {
    chrome::AddTabAt(second_browser, {}, -1, true);
  }
  histogram_tester_.ExpectBucketCount(kVerticalOpenTabsHistogramName, 2, 5);
  histogram_tester_.ExpectBucketCount(kVerticalOpenTabsHistogramName, 3, 1);

  SetVerticalTabsEnabled(false);

  for (size_t i = 0; i < 3; i++) {
    chrome::AddTabAt(browser(), {}, -1, true);
  }
  histogram_tester_.ExpectBucketCount(kVerticalOpenTabsHistogramName, 3, 1);
}

IN_PROC_BROWSER_TEST_F(VerticalTabMetricsTest, PinnedTabs) {
  chrome::AddTabAt(browser(), {}, -1, true);

  histogram_tester_.ExpectTotalCount(kVerticalPinnedTabsHistogramName, 0);

  SetVerticalTabsEnabled(true);

  for (size_t i = 0; i < 4; i++) {
    chrome::AddTabAt(browser(), {}, -1, true);
  }
  histogram_tester_.ExpectTotalCount(kVerticalPinnedTabsHistogramName, 0);

  TabStripModel* model = browser()->tab_strip_model();
  for (size_t i = 0; i < 2; i++) {
    model->SetTabPinned(i, true);
  }
  model->SelectNextTab();

  histogram_tester_.ExpectUniqueSample(kVerticalPinnedTabsHistogramName, 0, 1);

  model->SetTabPinned(2, true);
  model->SelectNextTab();

  histogram_tester_.ExpectBucketCount(kVerticalPinnedTabsHistogramName, 0, 1);
  histogram_tester_.ExpectBucketCount(kVerticalPinnedTabsHistogramName, 1, 1);

  for (size_t i = 0; i < 3; i++) {
    model->SetTabPinned(0, false);
  }
  model->SelectNextTab();

  histogram_tester_.ExpectBucketCount(kVerticalPinnedTabsHistogramName, 1, 4);

  SetVerticalTabsEnabled(false);
  for (size_t i = 0; i < 3; i++) {
    model->SetTabPinned(i, true);
  }
  model->SelectNextTab();
  histogram_tester_.ExpectTotalCount(kVerticalPinnedTabsHistogramName, 5);
}

IN_PROC_BROWSER_TEST_F(VerticalTabMetricsTest, GroupTabs) {
  chrome::AddTabAt(browser(), {}, -1, true);

  histogram_tester_.ExpectTotalCount(kVerticalGroupTabsHistogramName, 0);

  SetVerticalTabsEnabled(true);

  for (size_t i = 0; i < 4; i++) {
    chrome::AddTabAt(browser(), {}, -1, true);
  }
  histogram_tester_.ExpectTotalCount(kVerticalGroupTabsHistogramName, 0);

  TabStripModel* model = browser()->tab_strip_model();
  for (int i = 0; i < 2; i++) {
    model->AddToNewGroup({i});
  }
  model->SelectNextTab();

  histogram_tester_.ExpectUniqueSample(kVerticalGroupTabsHistogramName, 0, 1);

  model->AddToNewGroup({2});
  model->SelectNextTab();

  histogram_tester_.ExpectBucketCount(kVerticalGroupTabsHistogramName, 0, 1);
  histogram_tester_.ExpectBucketCount(kVerticalGroupTabsHistogramName, 1, 1);

  model->RemoveFromGroup({0, 1, 2});
  model->SelectNextTab();

  histogram_tester_.ExpectBucketCount(kVerticalGroupTabsHistogramName, 1, 2);

  SetVerticalTabsEnabled(false);

  for (int i = 0; i < 3; i++) {
    model->AddToNewGroup({i});
  }
  model->SelectNextTab();
  histogram_tester_.ExpectTotalCount(kVerticalGroupTabsHistogramName, 3);
}

}  // namespace misc_metrics
