// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/commands/browser/accelerator_pref_manager.h"

#include <string>

#include "brave/components/commands/common/accelerator_parsing.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

constexpr int kCommandIds[] = {1, 99, 100};

class AcceleratorPrefManagerTest : public testing::Test {
 public:
  AcceleratorPrefManagerTest() : manager_(&prefs_, kCommandIds) {
    commands::AcceleratorPrefManager::RegisterProfilePrefs(prefs_.registry());
  }

  ~AcceleratorPrefManagerTest() override = default;

  commands::AcceleratorPrefManager& manager() { return manager_; }

 private:
  TestingPrefServiceSimple prefs_;
  commands::AcceleratorPrefManager manager_;
};

TEST_F(AcceleratorPrefManagerTest, CanAddAccelerators) {
  ui::Accelerator accelerator1 = commands::FromCodesString("Shift+Alt+KeyC");
  EXPECT_EQ(0u, manager().GetAccelerators().size());
  manager().AddAccelerator(1, accelerator1);

  auto accelerators = manager().GetAccelerators();
  EXPECT_EQ(1u, accelerators.size());
  ASSERT_TRUE(accelerators.contains(1));
  ASSERT_EQ(1u, accelerators[1].size());
  EXPECT_EQ(accelerator1, accelerators[1][0]);

  ui::Accelerator accelerator2 = commands::FromCodesString("Ctrl+Cmd+KeyH");
  manager().AddAccelerator(1, accelerator2);

  accelerators = manager().GetAccelerators();
  EXPECT_EQ(1u, accelerators.size());
  ASSERT_TRUE(accelerators.contains(1));
  ASSERT_EQ(2u, accelerators[1].size());
  EXPECT_EQ(accelerator1, accelerators[1][0]);
  EXPECT_EQ(accelerator2, accelerators[1][1]);

  ui::Accelerator accelerator3 = commands::FromCodesString("Ctrl+KeyM");
  manager().AddAccelerator(100, accelerator3);

  accelerators = manager().GetAccelerators();
  EXPECT_EQ(2u, accelerators.size());
  ASSERT_TRUE(accelerators.contains(1));
  ASSERT_TRUE(accelerators.contains(100));
  ASSERT_EQ(2u, accelerators[1].size());
  EXPECT_EQ(accelerator1, accelerators[1][0]);
  EXPECT_EQ(accelerator2, accelerators[1][1]);
  ASSERT_EQ(1u, accelerators[100].size());
  EXPECT_EQ(accelerator3, accelerators[100][0]);
}

TEST_F(AcceleratorPrefManagerTest, CanRemoveAccelerators) {
  ui::Accelerator accelerator1 = commands::FromCodesString("Shift+Alt+KeyC");
  ui::Accelerator accelerator2 = commands::FromCodesString("Ctrl+Cmd+KeyH");
  ui::Accelerator accelerator3 = commands::FromCodesString("Ctrl+KeyM");
  manager().AddAccelerator(1, accelerator1);
  manager().AddAccelerator(1, accelerator2);
  manager().AddAccelerator(100, accelerator3);
  EXPECT_EQ(2u, manager().GetAccelerators().size());

  manager().RemoveAccelerator(1, accelerator1);

  auto accelerators = manager().GetAccelerators();
  EXPECT_EQ(2u, accelerators.size());
  ASSERT_TRUE(accelerators.contains(1));
  ASSERT_TRUE(accelerators.contains(100));
  ASSERT_EQ(1u, accelerators[1].size());
  EXPECT_EQ(accelerator2, accelerators[1][0]);
  ASSERT_EQ(1u, accelerators[100].size());
  EXPECT_EQ(accelerator3, accelerators[100][0]);

  manager().RemoveAccelerator(1, accelerator2);
  accelerators = manager().GetAccelerators();
  EXPECT_EQ(1u, accelerators.size());
  ASSERT_TRUE(accelerators.contains(100));
  ASSERT_EQ(1u, accelerators[100].size());
  EXPECT_EQ(accelerator3, accelerators[100][0]);

  // Removing accelerator which isn't on the command should have no effect
  manager().RemoveAccelerator(100, accelerator2);
  accelerators = manager().GetAccelerators();
  EXPECT_EQ(1u, accelerators.size());
  ASSERT_TRUE(accelerators.contains(100));
  ASSERT_EQ(1u, accelerators[100].size());
  EXPECT_EQ(accelerator3, accelerators[100][0]);

  // Removing accelerator from non-existent command should have no effect
  manager().RemoveAccelerator(99, accelerator2);
  accelerators = manager().GetAccelerators();
  EXPECT_EQ(1u, accelerators.size());
  ASSERT_TRUE(accelerators.contains(100));
  ASSERT_EQ(1u, accelerators[100].size());
  EXPECT_EQ(accelerator3, accelerators[100][0]);
}

TEST_F(AcceleratorPrefManagerTest, AcceleratorsAreNotDuplicated) {
  ui::Accelerator accelerator1 = commands::FromCodesString("Shift+Alt+KeyC");
  ui::Accelerator accelerator1_dupe =
      commands::FromCodesString("Shift+Alt+KeyC");
  ui::Accelerator accelerator2 = commands::FromCodesString("Ctrl+KeyM");
  manager().AddAccelerator(1, accelerator1);
  manager().AddAccelerator(1, accelerator1_dupe);
  manager().AddAccelerator(1, accelerator2);
  EXPECT_EQ(2u, manager().GetAccelerators()[1].size());
}
