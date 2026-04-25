/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ui/views/controls/button/md_text_button.h"

#include <memory>

#include "brave/ui/color/nala/nala_color_id.h"
#include "testing/gtest/include/gtest/gtest.h"
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
  button->SetStyle(ui::ButtonStyle::kProminent);

  auto* const native_theme = widget->GetNativeTheme();
  native_theme->set_preferred_color_scheme(ColorScheme::kLight);

  auto* color_provider = widget->GetColorProvider();

  // Explicitely set as active otherwise button could be in disabled state.
  test::WidgetTest::SimulateNativeActivate(widget.get());

  // Smoke test a few colors:
  EXPECT_EQ(color_provider->GetColor(nala::kColorButtonBackground),
            button->GetButtonColors().background_color);

  // Check that dark mode overrides are coming through:
  button->SetState(Button::ButtonState::STATE_HOVERED);
  EXPECT_EQ(color_provider->GetColor(nala::kColorPrimary60),
            button->GetButtonColors().background_color);

  native_theme->set_preferred_color_scheme(ColorScheme::kDark);

  // Fetch color provider after changing color scheme.
  color_provider = widget->GetColorProvider();

  EXPECT_EQ(color_provider->GetColor(nala::kColorPrimary50),
            button->GetButtonColors().background_color);

  // Check that setting Disabled adds opacity:
  button->SetState(Button::ButtonState::STATE_NORMAL);
  button->SetEnabled(false);
  EXPECT_EQ(color_provider->GetColor(nala::kColorButtonDisabled),
            button->GetButtonColors().background_color);
}

}  // namespace views
