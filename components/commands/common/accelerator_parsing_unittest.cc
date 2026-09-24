// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/commands/common/accelerator_parsing.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/accelerators/accelerator.h"

TEST(AcceleratorParsing, AcceleratorRoundTrips) {
  ui::Accelerator accelerator(ui::VKEY_C, ui::EF_SHIFT_DOWN | ui::EF_ALT_DOWN);

  auto serialized = commands::ToCodesString(accelerator);
  EXPECT_EQ("Alt+Shift+KeyC", serialized);

  auto parsed = commands::FromCodesString(serialized);
  EXPECT_EQ(accelerator, parsed);
}

TEST(AcceleratorParsing, AcceleratorKeyNamesAreFriendly) {
  ui::Accelerator accelerator(ui::VKEY_C, ui::EF_SHIFT_DOWN | ui::EF_ALT_DOWN);

  auto serialized = commands::ToKeysString(accelerator);
  EXPECT_EQ("Alt+Shift+c", serialized);
}
