// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_LEO_LEO_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_LEO_LEO_BUTTON_H_

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/controls/button/label_button.h"

namespace leo {

class LeoButton : public views::LabelButton {
 public:
  struct ButtonStyle {
    absl::optional<SkColor> background_color;
    absl::optional<SkColor> border_color;
    SkColor text_color;
  };

  struct ButtonTheme {
    ButtonStyle normal_light;
    ButtonStyle normal_dark;

    ButtonStyle hover_light;
    ButtonStyle hover_dark;

    ButtonStyle disabled_light;
    ButtonStyle disabled_dark;

    ButtonStyle loading_light;
    ButtonStyle loading_dark;
  };

  enum Kind { PRIMARY, SECONDARY, TERTIARY };

  explicit LeoButton(PressedCallback callback = PressedCallback(),
                     const std::u16string& text = std::u16string(),
                     int button_context = views::style::CONTEXT_BUTTON);
  LeoButton(const LeoButton&) = delete;
  LeoButton& operator=(const LeoButton&) = delete;
  ~LeoButton() override;

  void SetIcon(const gfx::VectorIcon* icon);

  void SetKind(Kind mode);
  Kind GetKind();

  void SetTheme(ButtonTheme theme);
  ButtonTheme GetTheme();

  bool IsLoading() { return loading_; }
  void SetLoading(bool loading);

  // views::LabelButton
  gfx::Insets GetInsets() const override;
  void StateChanged(ButtonState old_state) override;
  void OnThemeChanged() override;

 private:
  void UpdateTheme();
  void ApplyStyle(ButtonStyle style);

  raw_ptr<const gfx::VectorIcon> icon_ = nullptr;
  Kind mode_ = Kind::PRIMARY;
  ButtonTheme theme_;
  bool loading_ = false;
};

}  // namespace leo

#endif  // BRAVE_BROWSER_UI_VIEWS_LEO_LEO_BUTTON_H_
