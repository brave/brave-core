// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/ui/brave_custom_notification/padded_button.h"

#include <memory>

#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "brave/ui/brave_custom_notification/public/cpp/constants.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/painter.h"

namespace brave_custom_notification {

PaddedButton::PaddedButton(views::ButtonListener* listener)
    : views::ImageButton(listener) {
  SetFocusForPlatform();
  SetBackground(views::CreateSolidBackground(kControlButtonBackgroundColor));
  SetBorder(views::CreateEmptyBorder(gfx::Insets(kControlButtonBorderSize)));
  set_animate_on_state_change(false);

  SetInkDropMode(InkDropMode::ON);
  set_ink_drop_visible_opacity(0.12f);
  set_has_ink_drop_action_on_click(true);
}

std::unique_ptr<views::InkDrop> PaddedButton::CreateInkDrop() {
  auto ink_drop = CreateDefaultInkDropImpl();
  ink_drop->SetShowHighlightOnHover(false);
  ink_drop->SetShowHighlightOnFocus(false);
  return std::move(ink_drop);
}

void PaddedButton::OnThemeChanged() {
  ImageButton::OnThemeChanged();
  set_ink_drop_base_color(GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_PaddedButtonInkDropColor));
}

}  // namespace message_center
