// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/leo/leo_button.h"
#include <memory>
#include "include/core/SkColor.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view_class_properties.h"

namespace leo {
namespace colors {
SkColor kButtonPrimaryBackground = SkColorSetRGB(32, 74, 227);
SkColor kButtonPrimaryBackgroundHover = SkColorSetRGB(24, 56, 172);
SkColor kButtonPrimaryText = SK_ColorWHITE;

SkColor kButtonSecondaryBackgroundColorHover = SkColorSetRGB(221, 228, 251);
SkColor kButtonSecondaryText = SkColorSetRGB(107, 112, 132);
SkColor kButtonSecondaryTextHover = SkColorSetRGB(65, 101, 233);
SkColor kButtonSecondaryBorderColor = SkColorSetRGB(226, 227, 231);
SkColor kButtonSecondaryBorderColorHover = SkColorSetRGB(221, 228, 251);
}  // namespace colors

LeoButton::LeoButton(PressedCallback callback,
                     const std::u16string& text,
                     int button_context)
    : views::LabelButton(callback, text, button_context) {
  ApplyTheme();
}

LeoButton::~LeoButton() = default;

LeoButton::Mode LeoButton::GetMode() {
  return mode_;
}
void LeoButton::SetMode(Mode mode) {
  mode_ = mode;
  ApplyTheme();
}

void LeoButton::UpdateBackgroundColor() {
  auto state = GetVisualState();
  switch (mode_) {
    case PRIMARY:
      SetBackground(views::CreateRoundedRectBackground(
          state == ButtonState::STATE_HOVERED
              ? colors::kButtonPrimaryBackgroundHover
              : colors::kButtonPrimaryBackground,
          1000));
      break;
    case SECONDARY:
    case TERTIARY:
      SetBackground(nullptr);
      break;
  }
}

gfx::Insets LeoButton::GetInsets() const {
  return gfx::Insets::VH(6, 10);
}

void LeoButton::StateChanged(ButtonState old_state) {
  views::LabelButton::StateChanged(old_state);
  ApplyTheme();
}

void LeoButton::ApplyTheme() {
  switch (mode_) {
    case PRIMARY:
      ApplyPrimaryStyle();
      break;
    case SECONDARY:
      ApplySecondaryStyle();
      break;
    case TERTIARY:
      ApplyTertiaryStyle();
      break;
  }
  UpdateBackgroundColor();
}

void LeoButton::ApplyPrimaryStyle() {
  SetBorder(nullptr);
  SetEnabledTextColors(colors::kButtonPrimaryText);
}

void LeoButton::ApplySecondaryStyle() {
  auto state = GetVisualState();
  SetBorder(views::CreateRoundedRectBorder(
      1, 1000,
      state == ButtonState::STATE_HOVERED
          ? colors::kButtonSecondaryBorderColorHover
          : colors::kButtonSecondaryBorderColor));
  SetEnabledTextColors(colors::kButtonSecondaryText);
  SetTextColor(ButtonState::STATE_HOVERED, colors::kButtonSecondaryTextHover);
}

void LeoButton::ApplyTertiaryStyle() {}

}  // namespace leo
