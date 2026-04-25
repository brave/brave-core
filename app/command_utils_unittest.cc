// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/app/command_utils.h"

#include <algorithm>

#include "base/containers/flat_set.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/commands/common/features.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/accelerator_table.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// Note: If this test fails because an accelerated command isn't present just
// add the missing command to //brave/app/generate_command_metadata.py
TEST(CommandUtilsUnitTest, AllAcceleratedCommandsShouldBeAvailable) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(commands::features::kBraveCommands);

  auto accelerators = GetAcceleratorList();
  const auto& commands = commands::GetCommands();

  for (const auto& accelerator : accelerators) {
    EXPECT_TRUE(std::ranges::contains(commands, accelerator.command_id))
        << "Accelerated command '" << accelerator.command_id
        << "' was not present in the list of commands.";
  }
}

TEST(CommandUtilsUnitTest, NoTranslationsIncludeAmpersand) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(commands::features::kBraveCommands);

  for (const auto& command : commands::GetCommands()) {
    auto translation = commands::GetCommandName(command);
    EXPECT_THAT(translation, testing::Not(testing::HasSubstr("&")))
        << translation
        << " contains an '&' character. If this '&' is meant to be in the "
           "translation then this might be a false positive, in which case the "
           "test should be updated. The test is to ensure keyboard shortcuts "
           "from menus are not included in the name of commands.";
  }
}
