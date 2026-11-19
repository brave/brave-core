/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip_control_button.h"

#include "brave/browser/ui/toolbars_small_icons.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"

#include <chrome/browser/ui/views/tabs/tab_strip_control_button.cc>

TabStripControlButton::~TabStripControlButton() = default;

int TabStripControlButton::GetCornerRadius() const {
  // Ensure that tabstrip buttons have the correct rounded rect shape, and not
  // a circular shape.
  return ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, GetContentsBounds().size());
}

void TabStripControlButton::UpdateIcon() {
  TabStripControlButton_ChromiumImpl::UpdateIcon();

  // Upstream draws these icons at kIconSize (16).
  if (!brave::IsToolbarsSmallIconsEnabled()) {
    return;
  }
  for (const auto state :
       {views::Button::STATE_NORMAL, views::Button::STATE_HOVERED,
        views::Button::STATE_PRESSED}) {
    const auto& model = GetImageModel(state);
    if (!model || !model->IsVectorIcon()) {
      continue;
    }
    const ui::VectorIconModel vector_icon = model->GetVectorIcon();
    SetImageModel(state, ui::ImageModel::FromVectorIcon(
                             *vector_icon.vector_icon(), vector_icon.color(),
                             brave::kToolbarsSmallIconSize));
  }
}

BEGIN_METADATA(TabStripControlButton)
END_METADATA
