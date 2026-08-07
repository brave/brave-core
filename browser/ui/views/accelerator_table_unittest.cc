/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/accelerator_table.h"

#include <vector>

#include "base/containers/flat_set.h"
#include "brave/app/brave_command_ids.h"
#include "chrome/app/chrome_command_ids.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/event_constants.h"

namespace {

bool HasCommandID(int command_id) {
  const std::vector<AcceleratorMapping> accelerator_list(GetAcceleratorList());
  for (auto entry : accelerator_list) {
    if (entry.command_id == command_id)
      return true;
  }
  return false;
}

}  // namespace

TEST(AcceleratorTableTest, CheckBraveAccelerators) {
  EXPECT_TRUE(HasCommandID(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
  EXPECT_TRUE(HasCommandID(IDC_TOGGLE_SIDEBAR));
  EXPECT_TRUE(HasCommandID(IDC_NEW_SPLIT_TAB));
}

TEST(AcceleratorTableTest, CheckDuplicatedAccelerators) {
  base::flat_set<ui::Accelerator> accelerators;
  for (const auto& entry : GetAcceleratorList()) {
    EXPECT_TRUE(accelerators.emplace(entry.keycode, entry.modifiers).second)
        << "Duplicated accelerator: " << entry.keycode << ", "
        << (entry.modifiers & ui::EF_SHIFT_DOWN) << ", "
        << (entry.modifiers & ui::EF_CONTROL_DOWN) << ", "
        << (entry.modifiers & ui::EF_ALT_DOWN) << ", "
        << (entry.modifiers & ui::EF_ALTGR_DOWN);
  }
}
