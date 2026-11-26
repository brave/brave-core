/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_TEST_PINNED_TAB_RESET_BROWSERTEST_H_
#define BRAVE_BROWSER_UI_TABS_TEST_PINNED_TAB_RESET_BROWSERTEST_H_

#include "base/location.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "url/gurl.h"

class PinnedTabResetController;
class TabStripModel;

class PinnedTabResetBrowserTest : public InProcessBrowserTest {
 public:
  PinnedTabResetBrowserTest();
  ~PinnedTabResetBrowserTest() override;

  void SetUpOnMainThread() override;

 protected:
  TabStripModel* model();
  PinnedTabResetController* controller();

  // Pins the first tab at `pinned_url_`, adds an unpinned tab after it, and
  // navigates the pinned tab away from its pinned URL, leaving it active.
  void SetUpPinnedTabNavigatedAway();

  // Expects the tab at `index` to be at `pinned_url_` with no other history.
  void ExpectPinnedTabReset(
      int index = 0,
      base::Location location = base::Location::Current());

  GURL pinned_url_;
  GURL other_url_;

 private:
  base::test::ScopedFeatureList feature_list_;
};

#endif  // BRAVE_BROWSER_UI_TABS_TEST_PINNED_TAB_RESET_BROWSERTEST_H_
