// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/views/controls/button/md_text_button.h"

TEST(MdTextButtonConstantsTest, ExpectedAlphaBlendConstants) {
  // This test verifies that the constants used in the MdTextButton internally
  // match the expected alpha blend results. In case these test fails, it
  // indicates that the values in `gfx` potentially changed, and the constants
  // in `views` need to be updated accordingly, based on the new alpha blend
  // results emitted by the test failure.
  EXPECT_EQ(
      color_utils::AlphaBlend(SK_ColorBLACK, gfx::kColorButtonBackground, 0.2f),
      views::internal::kColorButtonBackgroundBlack)
      << "kColorButtonBackgroundBlack needs to be updated.";
  EXPECT_EQ(
      color_utils::AlphaBlend(SK_ColorWHITE, gfx::kColorButtonBackground, 0.2f),
      views::internal::kColorButtonBackgroundWhite)
      << "kColorButtonBackgroundWhite needs to be updated.";
  EXPECT_EQ(color_utils::AlphaBlend(SK_ColorBLACK,
                                    gfx::kColorDividerInteractive, 0.2f),
            views::internal::kColorDividerInteractiveBlack)
      << "kColorDividerInteractiveBlack needs to be updated.";
  EXPECT_EQ(
      color_utils::AlphaBlend(SK_ColorBLACK, gfx::kColorTextInteractive, 0.2f),
      views::internal::kColorTextInteractiveBlack)
      << "kColorTextInteractiveBlack needs to be updated.";
  EXPECT_EQ(color_utils::AlphaBlend(SK_ColorWHITE,
                                    gfx::kColorDividerInteractive, 0.2f),
            views::internal::kColorDividerInteractiveWhite)
      << "kColorDividerInteractiveWhite needs to be updated.";
  EXPECT_EQ(color_utils::AlphaBlend(SK_ColorWHITE,
                                    gfx::kColorTextInteractiveDark, 0.2f),
            views::internal::kColorTextInteractiveDarkWhite)
      << "kColorTextInteractiveDarkWhite needs to be updated.";
  EXPECT_EQ(
      color_utils::AlphaBlend(SK_ColorBLACK, gfx::kColorTextSecondary, 0.2f),
      views::internal::kColorTextSecondaryBlack)
      << "kColorTextSecondaryBlack needs to be updated.";
  EXPECT_EQ(color_utils::AlphaBlend(SK_ColorWHITE, gfx::kColorTextSecondaryDark,
                                    0.2f),
            views::internal::kColorTextSecondaryDarkWhite)
      << "kColorTextSecondaryDarkWhite needs to be updated.";
}
