/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/ui/views/frame/browser_frame_view_layout_linux.cc"

#include "base/check_is_test.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "ui/views/window/caption_button_layout_constants.h"
#include "ui/views/window/frame_caption_button.h"

void BrowserFrameViewLayoutLinux::SetBoundsForButton(
    views::FrameButton button_id,
    views::Button* button,
    ButtonAlignment align) {
  OpaqueBrowserFrameViewLayout::SetBoundsForButton(button_id, button, align);
  if (!view_) {
    CHECK_IS_TEST();
    return;
  }

  auto* browser = view_->browser_view()->browser();
  DCHECK(browser);

  const bool should_window_caption_buttons_overlap_toolbar =
      tabs::utils::ShouldShowVerticalTabs(browser) &&
      !tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser);
  if (!should_window_caption_buttons_overlap_toolbar) {
    return;
  }

  if (delegate_->GetFrameButtonStyle() ==
      OpaqueBrowserFrameViewLayoutDelegate::FrameButtonStyle::kMdButton) {
    // Synchronize frame button's bounds with toolbar's bounds.
    gfx::Size size = button->GetPreferredSize();
    DCHECK_LT(0, size.width());
    auto* toolbar = view_->browser_view()->toolbar();
    const auto toolbar_height = toolbar->GetPreferredSize().height();
    size.set_height(toolbar_height);
    button->SetPreferredSize(size);
    button->SetSize(size);
    gfx::Point toolbar_origin;
    views::View::ConvertPointToTarget(toolbar, button->parent(),
                                      &toolbar_origin);
    button->SetY(toolbar_origin.y());

    static_cast<views::FrameCaptionButton*>(button)->SetInkDropCornerRadius(
        views::kCaptionButtonInkDropDefaultCornerRadius);
  }
}
