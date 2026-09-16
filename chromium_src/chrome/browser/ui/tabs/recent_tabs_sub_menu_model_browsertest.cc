/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/containers/to_vector.h"

// The 13 previously-disabled tests in this file (g_brave_browser_process
// crashes, menu-index mismatches, Brave Sync's own auth, etc.) are now
// disabled via test/filters/browser_tests.filter per CSRC-002 instead of
// chromium_src #define renames.

// The case when number of tabs on other device is <=4 so we do not add
// `More...` item is tested by RecentTabsSubMenuModelTest.MaxSessionsAndRecency

#include <chrome/browser/ui/tabs/recent_tabs_sub_menu_model_browsertest.cc>

// This override is in place because we must adjust the menu model to match our
// expectations
void RecentTabsSubMenuModelTest::VerifyModel(
    const RecentTabsSubMenuModel& model,
    base::span<const ModelData> input) {
  // We have to copy it over as we can not modify the input.
  auto data = base::ToVector(input);

  // We replace the "Sign in to see tabs from other devices" menu command with
  // the non-command string "No tabs from other devices" and need to adjust the
  // data
  auto& item_data = data.back();
  if (item_data.type == ui::MenuModel::TYPE_COMMAND) {
    item_data.enabled = false;
  }

  // The first two commands are History and History Clusters, but we disable
  // History Clusters and upstream won't show it, so we should skip one command.
  ::VerifyModel(model, base::span(data).subspan(1u));
}

void RecentTabsSubMenuModelTest::VerifyModel(const ui::MenuModel* model,
                                             base::span<const ModelData> data) {
  ::VerifyModel(model, data);
}
