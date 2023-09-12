/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/ui/views/frame/browser_frame_view_layout_linux.cc"

#include "base/check_is_test.h"
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

  // Sets fixed heights for |button| regardless of non client top height.
  if (delegate_->GetFrameButtonStyle() ==
      OpaqueBrowserFrameViewLayoutDelegate::FrameButtonStyle::kMdButton) {
    gfx::Size size = button->GetPreferredSize();
    DCHECK_LT(0, size.width());
    size.set_height(size.width());
    button->SetPreferredSize(size);
    button->SetSize(size);
    const auto toolbar_height =
        view_->browser_view()->toolbar()->GetPreferredSize().height();
    button->SetY(view_->GetTopAreaHeight() +
                 (toolbar_height - size.height()) / 2);
    static_cast<views::FrameCaptionButton*>(button)->SetInkDropCornerRadius(
        views::kCaptionButtonInkDropDefaultCornerRadius);
  }
}
