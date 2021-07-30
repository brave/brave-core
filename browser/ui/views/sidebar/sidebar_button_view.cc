/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "ui/gfx/color_palette.h"
#include "ui/views/controls/focus_ring.h"

SidebarButtonView::SidebarButtonView(Delegate* delegate) : delegate_(delegate) {
  // Locate image at center of the button.
  SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
  SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  DCHECK(GetInstallFocusRingOnFocus());
  focus_ring()->SetColor(gfx::kBraveBlurple300);
}

SidebarButtonView::~SidebarButtonView() = default;

gfx::Size SidebarButtonView::CalculatePreferredSize() const {
  return {kSidebarButtonSize, kSidebarButtonSize};
}

std::u16string SidebarButtonView::GetTooltipText(const gfx::Point& p) const {
  if (!delegate_)
    return views::ImageButton::GetTooltipText(p);

  return delegate_->GetTooltipTextFor(this);
}
