/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

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

#define BRAVE_RECENT_TABS_SUB_MENU_MODEL_TEST           \
  void VerifyModel(const RecentTabsSubMenuModel& model, \
                   base::span<const ModelData> data);   \
  void VerifyModel(const ui::MenuModel* model,          \
                   base::span<const ModelData> data);

// The case when number of tabs on other device is <=4 so we do not add
// `More...` item is tested by RecentTabsSubMenuModelTest.MaxSessionsAndRecency

#include "src/chrome/browser/ui/toolbar/recent_tabs_sub_menu_model_unittest.cc"

#undef BRAVE_RECENT_TABS_SUB_MENU_MODEL_TEST

#undef RecentlyClosedTabsAndWindowsFromLastSession
#undef RecentlyClosedTabsFromCurrentSession
#undef RecentlyClosedGroupsFromCurrentSession

void RecentTabsSubMenuModelTest::VerifyModel(
    const RecentTabsSubMenuModel& model,
    base::span<const ModelData> data) {
  std::vector<ModelData> v_data{data.begin(), data.end()};
  v_data.insert(v_data.begin() + 1, {ui::MenuModel::TYPE_COMMAND, true});
  const std::string_view test_name =
      testing::UnitTest::GetInstance()->current_test_info()->name();
  if (test_name == "MaxTabsPerSessionAndRecency") {
    v_data.push_back({ui::MenuModel::TYPE_COMMAND, true});
  }
  ::VerifyModel(model, base::make_span(v_data.begin(), v_data.size()));
}

void RecentTabsSubMenuModelTest::VerifyModel(const ui::MenuModel* model,
                                             base::span<const ModelData> data) {
  ::VerifyModel(model, data);
}
