// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/leo/leo_button.h"
#include <memory>
#include <sstream>
#include <string>
#include "absl/types/optional.h"
#include "include/core/SkColor.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view_class_properties.h"

namespace leo {
namespace colors {

LeoButton::ButtonTheme g_primary_theme = {
    LeoButton::ButtonStyle{SkColorSetRGB(32, 74, 227), absl::nullopt,
                           SK_ColorWHITE},
    LeoButton::ButtonStyle{SkColorSetRGB(24, 56, 172), absl::nullopt,
                           SK_ColorWHITE}};

LeoButton::ButtonTheme g_secondary_theme = {
    LeoButton::ButtonStyle{absl::nullopt, SkColorSetRGB(226, 227, 231),
                           SkColorSetRGB(107, 112, 132)},
    LeoButton::ButtonStyle{SkColorSetRGB(243, 245, 254),
                           SkColorSetRGB(221, 228, 251),
                           SkColorSetRGB(65, 101, 233)}};

LeoButton::ButtonTheme g_tertiary_theme = {};

}  // namespace colors

LeoButton::LeoButton(PressedCallback callback,
                     const std::u16string& text,
                     int button_context)
    : views::LabelButton(callback, text, button_context),
      theme_(colors::g_primary_theme) {
  UpdateTheme();
}

LeoButton::~LeoButton() = default;

LeoButton::Kind LeoButton::GetKind() {
  return mode_;
}
void LeoButton::SetKind(Kind mode) {
  mode_ = mode;
  if (mode == Kind::PRIMARY)
    theme_ = colors::g_primary_theme;
  if (mode == Kind::SECONDARY)
    theme_ = colors::g_secondary_theme;
  if (mode == Kind::TERTIARY)
    theme_ = colors::g_tertiary_theme;
  UpdateTheme();
}

gfx::Insets LeoButton::GetInsets() const {
  return gfx::Insets::VH(6, 10);
}

void LeoButton::StateChanged(ButtonState old_state) {
  views::LabelButton::StateChanged(old_state);
  UpdateTheme();
}

LeoButton::ButtonTheme LeoButton::GetTheme() {
  return theme_;
}
void LeoButton::SetTheme(ButtonTheme theme) {
  theme_ = theme;
  UpdateTheme();
}

void LeoButton::UpdateTheme() {
  auto state = GetVisualState();
  auto style = theme_.normal;
  if (state == STATE_HOVERED)
    style = theme_.hover;
  ApplyStyle(style);
}

void echo_color(std::string name, absl::optional<SkColor> c) {
  if (!c.has_value()) {
    LOG(ERROR) << name << ": undefined";
    return;
  }
  LOG(ERROR) << name << ": rgb(" << SkColorGetR(c.value()) << ", "
             << SkColorGetG(c.value()) << ", " << SkColorGetB(c.value()) << ")";
}

void LeoButton::ApplyStyle(ButtonStyle style) {
  LOG(ERROR) << "===Start Style===";
  echo_color("Text", style.text_color);
  echo_color("Backgroudn", style.background_color);
  echo_color("Border", style.border_color);
  LOG(ERROR) << "====End Style====";

  SetBackground(style.background_color.has_value()
                    ? views::CreateRoundedRectBackground(
                          style.background_color.value(), 1000)
                    : nullptr);
  SetBorder(
      style.border_color.has_value()
          ? views::CreateRoundedRectBorder(1, 1000, style.border_color.value())
          : nullptr);

  SetEnabledTextColors(style.text_color);
}

}  // namespace leo
