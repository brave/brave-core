/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_LABEL_BUTTON_H_
#define BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_LABEL_BUTTON_H_

#include "ui/views/controls/button/label_button.h"

namespace brave_tooltips {

class BraveTooltipLabelButton : public views::LabelButton {
 public:
  // Creates a BraveTooltipLabelButton with pressed events sent to |callback|
  // and label |text|. |button_context| is a value from
  // views::style::TextContext and determines the appearance of |text|.
  explicit BraveTooltipLabelButton(
      PressedCallback callback = PressedCallback(),
      const std::u16string& text = std::u16string(),
      int button_context = views::style::CONTEXT_BUTTON);
  ~BraveTooltipLabelButton() override;

  BraveTooltipLabelButton(const BraveTooltipLabelButton&) = delete;
  BraveTooltipLabelButton& operator=(const BraveTooltipLabelButton&) = delete;

  // views::LabelButton:
  gfx::NativeCursor GetCursor(const ui::MouseEvent& event) override;
};

}  // namespace brave_tooltips

#endif  // BRAVE_BROWSER_UI_BRAVE_TOOLTIPS_BRAVE_TOOLTIP_LABEL_BUTTON_H_
