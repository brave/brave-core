// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commands/default_accelerators.h"

#include <algorithm>

#include "build/build_config.h"
#include "chrome/browser/ui/accelerator_table.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/accelerators/accelerator.h"

#if BUILDFLAG(IS_MAC)
#include "chrome/app/chrome_command_ids.h"
#endif  // BUILDFLAG(IS_MAC)

namespace commands {

TEST(DefaultAcceleratorsUnitTest, IncludesAcceleratorTable) {
  DefaultAccelerators defaults = GetDefaultAccelerators();
  for (const AcceleratorMapping& mapping : GetAcceleratorList()) {
    EXPECT_TRUE(std::ranges::contains(
        defaults.accelerators[mapping.command_id],
        ui::Accelerator(mapping.keycode, mapping.modifiers)))
        << "Missing default accelerator for command " << mapping.command_id;
  }
}

#if BUILDFLAG(IS_MAC)
TEST(DefaultAcceleratorsUnitTest, MenuDispatchedAcceleratorsAreDefaults) {
  DefaultAccelerators defaults = GetDefaultAccelerators();
  // Every menu dispatched accelerator must also be a default accelerator of
  // the same command, as it's dispatched by the menu while assigned to it.
  for (const auto& [command_id, accelerators] : defaults.menu_dispatched) {
    for (const auto& accelerator : accelerators) {
      EXPECT_TRUE(
          std::ranges::contains(defaults.accelerators[command_id], accelerator))
          << "Menu dispatched accelerator isn't a default for command "
          << command_id;
    }
  }
}

TEST(DefaultAcceleratorsUnitTest, SystemManagedAcceleratorsAreDefaults) {
  const DefaultAccelerators defaults = GetDefaultAccelerators();
  for (const auto& accelerator : defaults.system_managed) {
    EXPECT_TRUE(std::ranges::any_of(
        defaults.accelerators, [&accelerator](const auto& entry) {
          return std::ranges::contains(entry.second, accelerator);
        }))
        << "System managed accelerator isn't a default for any command";
  }
}

TEST(DefaultAcceleratorsUnitTest, CloseTabAndCloseWindowAreSystemManaged) {
  // IDC_CLOSE_TAB / IDC_CLOSE_WINDOW key equivalents are hard-coded and
  // dynamically swapped by upstream's app_controller_mac.mm, so their default
  // shortcuts must be system managed (not customizable).
  DefaultAccelerators defaults = GetDefaultAccelerators();
  // There's no main menu in unit tests, but IDC_CLOSE_TAB's shortcut comes
  // from the static AcceleratorsCocoa table. IDC_CLOSE_WINDOW's shortcut is
  // only derived from its menu item, so it has no accelerators here.
  ASSERT_FALSE(defaults.accelerators[IDC_CLOSE_TAB].empty());
  for (int command_id : {IDC_CLOSE_TAB, IDC_CLOSE_WINDOW}) {
    for (const auto& accelerator : defaults.accelerators[command_id]) {
      EXPECT_TRUE(defaults.system_managed.contains(accelerator))
          << "Accelerator for command " << command_id
          << " isn't system managed";
    }
  }
}
#endif  // BUILDFLAG(IS_MAC)

}  // namespace commands
