// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/leo/leo_button.h"
#include <memory>
#include <sstream>
#include <string>
#include "absl/types/optional.h"
#include "base/bind.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "include/core/SkColor.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/view_class_properties.h"

namespace leo {
namespace colors {

LeoButton::ButtonTheme g_primary_theme = {
    // Normal Light
    LeoButton::ButtonStyle{SkColorSetRGB(32, 74, 227), absl::nullopt,
                           SK_ColorWHITE},
    // Normal Dark
    LeoButton::ButtonStyle{SkColorSetRGB(32, 74, 227), absl::nullopt,
                           SK_ColorWHITE},
    // Hover Light
    LeoButton::ButtonStyle{SkColorSetRGB(24, 56, 172), absl::nullopt,
                           SK_ColorWHITE},
    // Hover Dark
    LeoButton::ButtonStyle{SkColorSetRGB(77, 92, 253), absl::nullopt,
                           SK_ColorWHITE},
    // Disabled Light
    LeoButton::ButtonStyle{SkColorSetARGB(128, 172, 175, 187), absl::nullopt,
                           SkColorSetA(SK_ColorWHITE, 128)},
    // Disabled Dark
    LeoButton::ButtonStyle{SkColorSetARGB(128, 88, 92, 109), absl::nullopt,
                           SkColorSetA(SK_ColorWHITE, 128)},
    // Loading Light
    LeoButton::ButtonStyle{SkColorSetARGB(192, 32, 74, 227), absl::nullopt,
                           SkColorSetA(SK_ColorWHITE, 192)},
    // Loading Dark
    LeoButton::ButtonStyle{SkColorSetARGB(192, 32, 74, 227), absl::nullopt,
                           SkColorSetA(SK_ColorWHITE, 192)}};

LeoButton::ButtonTheme g_secondary_theme = {
    // Normal Light
    LeoButton::ButtonStyle{absl::nullopt, SkColorSetRGB(226, 227, 231),
                           SkColorSetRGB(107, 112, 132)},
    // Normal Dark
    LeoButton::ButtonStyle{absl::nullopt, SkColorSetRGB(46, 48, 57),
                           SkColorSetRGB(140, 144, 161)},
    // Hover Light
    LeoButton::ButtonStyle{SkColorSetRGB(243, 245, 254),
                           SkColorSetRGB(221, 228, 251),
                           SkColorSetRGB(65, 101, 233)},
    // Hover Dark
    LeoButton::ButtonStyle{SkColorSetRGB(7, 16, 50), SkColorSetRGB(17, 39, 121),
                           SkColorSetRGB(153, 173, 243)},
    // Disabled Light
    LeoButton::ButtonStyle{absl::nullopt, SkColorSetARGB(128, 226, 227, 231),
                           SkColorSetARGB(128, 107, 112, 132)},
    // Disabled Dark
    LeoButton::ButtonStyle{absl::nullopt, SkColorSetARGB(128, 46, 48, 57),
                           SkColorSetARGB(128, 140, 144, 161)},
    // Loading Light
    LeoButton::ButtonStyle{absl::nullopt, SkColorSetARGB(192, 226, 227, 231),
                           SkColorSetARGB(192, 107, 112, 132)},
    // Loading Dark
    LeoButton::ButtonStyle{absl::nullopt, SkColorSetARGB(192, 46, 48, 57),
                           SkColorSetARGB(192, 140, 144, 161)}};

LeoButton::ButtonTheme g_tertiary_theme = {
    // Normal Light
    LeoButton::ButtonStyle{absl::nullopt, absl::nullopt,
                           SkColorSetRGB(32, 74, 227)},
    // Normal Dark
    LeoButton::ButtonStyle{absl::nullopt, absl::nullopt,
                           SkColorSetRGB(153, 173, 243)},
    // Hover Light
    LeoButton::ButtonStyle{absl::nullopt, absl::nullopt,
                           SkColorSetRGB(24, 56, 172)},
    // Hover Dark
    LeoButton::ButtonStyle{absl::nullopt, absl::nullopt,
                           SkColorSetRGB(186, 199, 247)},
};

}  // namespace colors

LeoButton::LeoButton(PressedCallback callback,
                     const std::u16string& text,
                     int button_context)
    : views::LabelButton(callback, text, button_context),
      theme_(colors::g_primary_theme) {
  UpdateTheme();
  SetImageLabelSpacing(6);

  DCHECK(AddEnabledChangedCallback(
      base::BindRepeating(&LeoButton::UpdateTheme, base::Unretained(this))));
}

LeoButton::~LeoButton() = default;

void LeoButton::SetIcon(const gfx::VectorIcon* icon) {
  icon_ = icon;
  if (icon_)
    UpdateTheme();
  else
    image()->SetImage(nullptr);
}

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

LeoButton::ButtonTheme LeoButton::GetTheme() {
  return theme_;
}

void LeoButton::SetTheme(ButtonTheme theme) {
  theme_ = theme;
  UpdateTheme();
}

void LeoButton::SetLoading(bool loading) {
  loading_ = loading;
  UpdateTheme();
}

gfx::Insets LeoButton::GetInsets() const {
  return gfx::Insets::VH(6, 10);
}

void LeoButton::StateChanged(ButtonState old_state) {
  views::LabelButton::StateChanged(old_state);
  UpdateTheme();
}

void LeoButton::OnThemeChanged() {
  views::LabelButton::OnThemeChanged();
  UpdateTheme();
}

void LeoButton::UpdateTheme() {
  auto state = GetVisualState();
  auto is_dark = dark_mode::GetActiveBraveDarkModeType() ==
                 dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  auto style = is_dark ? theme_.normal_dark : theme_.normal_light;
  if (state == STATE_HOVERED)
    style = is_dark ? theme_.hover_dark : theme_.hover_light;

  if (loading_)
    style = is_dark ? theme_.loading_dark : theme_.loading_light;
  if (!GetEnabled())
    style = is_dark ? theme_.disabled_dark : theme_.disabled_light;

  ApplyStyle(style);
}

void LeoButton::ApplyStyle(ButtonStyle style) {
  SetBackground(style.background_color.has_value()
                    ? views::CreateRoundedRectBackground(
                          style.background_color.value(), 1000)
                    : nullptr);
  SetBorder(
      style.border_color.has_value()
          ? views::CreateRoundedRectBorder(2, 1000, style.border_color.value())
          : nullptr);

  SetEnabledTextColors(style.text_color);

  if (icon_) {
    SetImage(ButtonState::STATE_NORMAL,
             gfx::CreateVectorIcon(*icon_, style.text_color));
  }
}

}  // namespace leo
