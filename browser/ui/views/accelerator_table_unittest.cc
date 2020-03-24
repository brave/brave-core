/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "brave/app/brave_command_ids.h"
#include "chrome/browser/ui/views/accelerator_table.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

bool HasCommandID(int command_id) {
  const std::vector<AcceleratorMapping> accelerator_list(GetAcceleratorList());
  for (auto it = accelerator_list.begin(); it != accelerator_list.end(); ++it) {
    const AcceleratorMapping& entry = *it;
    if (entry.command_id == command_id)
      return true;
  }
  return false;
}

}  // namespace

TEST(AcceleratorTableTest, CheckBraveAccelerators) {
  EXPECT_TRUE(HasCommandID(IDC_NEW_OFFTHERECORD_WINDOW_TOR));
}
