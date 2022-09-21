// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_LEO_LEO_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_LEO_LEO_BUTTON_H_

#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/label_button.h"

namespace leo {

class LeoButton : public views::LabelButton {
 public:
  enum Mode { PRIMARY, SECONDARY, TERTIARY };

  explicit LeoButton(PressedCallback callback = PressedCallback(),
                     const std::u16string& text = std::u16string(),
                     int button_context = views::style::CONTEXT_BUTTON);
  LeoButton(const LeoButton&) = delete;
  LeoButton& operator=(const LeoButton&) = delete;
  ~LeoButton() override;

  void SetMode(Mode mode);
  Mode GetMode();

  // views::LabelButton
  void UpdateBackgroundColor() override;
  gfx::Insets GetInsets() const override;
  void StateChanged(ButtonState old_state) override;

 private:
  void ApplyTheme();
  void ApplyPrimaryStyle();
  void ApplySecondaryStyle();
  void ApplyTertiaryStyle();

  Mode mode_ = Mode::PRIMARY;
};

}  // namespace leo

#endif  // BRAVE_BROWSER_UI_VIEWS_LEO_LEO_BUTTON_H_
