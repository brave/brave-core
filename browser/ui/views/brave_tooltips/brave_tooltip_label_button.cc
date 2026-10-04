/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_tooltips/brave_tooltip_label_button.h"

#include <utility>

#include "ui/base/cursor/cursor.h"

namespace brave_tooltips {

BraveTooltipLabelButton::BraveTooltipLabelButton(PressedCallback callback,
                                                 const std::u16string& text,
                                                 int button_context)
    : LabelButton(std::move(callback), text, button_context) {}

BraveTooltipLabelButton::~BraveTooltipLabelButton() = default;

ui::Cursor BraveTooltipLabelButton::GetCursor(const ui::MouseEvent& event) {
  if (!GetEnabled())
    return ui::Cursor();
  return ui::mojom::CursorType::kHand;
}

}  // namespace brave_tooltips
