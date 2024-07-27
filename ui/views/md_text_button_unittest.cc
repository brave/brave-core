/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ui/views/controls/button/md_text_button.h"

#include <memory>

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/color_palette.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/test/widget_test.h"

namespace views {

using ColorScheme = ui::NativeTheme::PreferredColorScheme;
using MdTextButtonTest = ViewsTestBase;

TEST_F(MdTextButtonTest, ButtonColorsTest) {
  std::unique_ptr<Widget> widget =
      CreateTestWidget(views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET);

  auto* button = widget->SetContentsView(
      std::make_unique<MdTextButton>(Button::PressedCallback(), u" "));
  button->SetEnabled(false);

  // Check proper text color is used for each theme options.
  auto* const native_theme = widget->GetNativeTheme();
  native_theme->set_preferred_color_scheme(ColorScheme::kLight);
  EXPECT_EQ(gfx::kColorTextDisabled, button->GetButtonColors().text_color);

  native_theme->set_preferred_color_scheme(ColorScheme::kDark);
  EXPECT_EQ(gfx::kColorTextDisabledDark, button->GetButtonColors().text_color);
}

}  // namespace views
