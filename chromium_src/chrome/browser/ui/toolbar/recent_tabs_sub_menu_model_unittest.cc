/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/strings/string_util.h"

// Disabling these tests because they refer g_brave_browser_process which is not
// initialized in unit tests, is null and so they are crashing.
// Not related to change in RecentTabsSubMenuModel for additional `More...`
// menu item
#define RecentlyClosedTabsFromCurrentSession \
  DISABLED_RecentlyClosedTabsFromCurrentSession

#define RecentlyClosedTabsAndWindowsFromLastSession \
  DISABLED_RecentlyClosedTabsAndWindowsFromLastSession

#define RecentlyClosedGroupsFromCurrentSession \
  DISABLED_RecentlyClosedGroupsFromCurrentSession

// Disabling these tests because they reference items in menu explicitly by
// index which doesn't match our menu since we insert an additional "Clear
// browsing data" item ahead of the entries of interest to the test.
#define MaxSessionsAndRecency DISABLED_MaxSessionsAndRecency

// Disabling these tests because our "More..." item doesn't match the test's
// expectation of the tab name.
#define MaxTabsPerSessionAndRecency DISABLED_MaxTabsPerSessionAndRecency

// Disabling this because we have refresh disabled, but it fails because
// ExtensionWebContentsObserver::GetForWebContents returns nullptr and makes
// the test crash.
#define RecentlyClosedTabsAndWindowsFromLastSessionWithRefresh \
  DISABLED_RecentlyClosedTabsAndWindowsFromLastSessionWithRefresh

#define BRAVE_RECENT_TABS_SUB_MENU_MODEL_TEST           \
  void VerifyModel(const RecentTabsSubMenuModel& model, \
                   base::span<const ModelData> data);   \
  void VerifyModel(const ui::MenuModel* model,          \
                   base::span<const ModelData> data);

// The case when number of tabs on other device is <=4 so we do not add
// `More...` item is tested by RecentTabsSubMenuModelTest.MaxSessionsAndRecency

#include "src/chrome/browser/ui/toolbar/recent_tabs_sub_menu_model_unittest.cc"

#undef BRAVE_RECENT_TABS_SUB_MENU_MODEL_TEST

#undef RecentlyClosedTabsAndWindowsFromLastSessionWithRefresh
#undef MaxTabsPerSessionAndRecency
#undef MaxSessionsAndRecency
#undef RecentlyClosedTabsAndWindowsFromLastSession
#undef RecentlyClosedTabsFromCurrentSession
#undef RecentlyClosedGroupsFromCurrentSession

// This function override is in place here to make sure we insert an extra entry
// on the model data to be checked, representing Brave's entry "Clear Browsing
// Data".
void RecentTabsSubMenuModelTest::VerifyModel(
    const RecentTabsSubMenuModel& model,
    base::span<const ModelData> data) {
  std::vector<ModelData> v_data{data.begin(), data.end()};
  v_data.insert(v_data.begin() + 1, {ui::MenuModel::TYPE_COMMAND, true});
  ::VerifyModel(model, base::make_span(v_data.begin(), v_data.size()));
}

void RecentTabsSubMenuModelTest::VerifyModel(const ui::MenuModel* model,
                                             base::span<const ModelData> data) {
  ::VerifyModel(model, data);
}
