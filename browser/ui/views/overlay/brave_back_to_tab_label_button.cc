/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/overlay/brave_back_to_tab_label_button.h"

#include <utility>

#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/background.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/layout/layout_provider.h"

BraveBackToTabLabelButton::BraveBackToTabLabelButton(PressedCallback callback)
    : BackToTabLabelButton(std::move(callback)) {
  // Align this button's style with OverlayWindowImageButton.
  views::InkDrop::Get(this)->SetMode(views::InkDropHost::InkDropMode::ON);
  SetHasInkDropActionOnClick(true);

  views::InstallCircleHighlightPathGenerator(this);
  SetInstallFocusRingOnFocus(true);

  SetImageLabelSpacing(0);
  SetImageCentered(true);

  const auto insets = views::LayoutProvider::Get()->GetInsetsMetric(
      views::INSETS_VECTOR_IMAGE_BUTTON);
  SetBorder(views::CreateEmptyBorder(insets));
}

BraveBackToTabLabelButton::~BraveBackToTabLabelButton() = default;

void BraveBackToTabLabelButton::OnThemeChanged() {
  BackToTabLabelButton::OnThemeChanged();

  views::InkDrop::Get(this)->SetBaseColor(
      GetColorProvider()->GetColor(kColorPipWindowForeground));
  SetBackground(nullptr);
}

BEGIN_METADATA(BraveBackToTabLabelButton)
END_METADATA
