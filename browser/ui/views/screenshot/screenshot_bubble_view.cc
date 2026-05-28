// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/screenshot/screenshot_bubble_view.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/ui/screenshot/screenshot_controller.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/color/color_id.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace brave {

namespace {

constexpr int kBubbleWidth = 280;
constexpr int kRowSpacing = 8;
constexpr int kIconSize = 16;

}  // namespace

// static
views::Widget* ScreenshotBubbleView::Show(Browser* browser,
                                          views::View* anchor,
                                          ScreenshotController* controller) {
  auto bubble =
      std::make_unique<ScreenshotBubbleView>(browser, anchor, controller);
  views::Widget* widget =
      views::BubbleDialogDelegateView::CreateBubble(std::move(bubble));
  widget->Show();
  return widget;
}

ScreenshotBubbleView::ScreenshotBubbleView(Browser* browser,
                                           views::View* anchor,
                                           ScreenshotController* controller)
    : BubbleDialogDelegateView(anchor, views::BubbleBorder::TOP_RIGHT),
      browser_(browser),
      controller_(controller) {
  SetTitle(l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_BUBBLE_TITLE));
  SetShowCloseButton(true);
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
  set_fixed_width(kBubbleWidth);
  set_margins(gfx::Insets::TLBR(0, 16, 16, 16));

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      /*inside_border_insets=*/gfx::Insets(),
      /*between_child_spacing=*/kRowSpacing));

  visible_area_button_ = AddChildView(std::make_unique<views::MdTextButton>(
      base::BindRepeating(&ScreenshotBubbleView::OnVisibleAreaPressed,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_VISIBLE_AREA)));
  visible_area_button_->SetImageModel(
      views::Button::ButtonState::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoScreenshotIcon,
                                     ui::kColorButtonForeground, kIconSize));

  full_page_button_ = AddChildView(std::make_unique<views::MdTextButton>(
      base::BindRepeating(&ScreenshotBubbleView::OnFullPagePressed,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_FULL_PAGE)));
  full_page_button_->SetImageModel(
      views::Button::ButtonState::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoFullscreenOnIcon,
                                     ui::kColorButtonForeground, kIconSize));

  const bool can_capture =
      controller_ && controller_->CanCapture(GetActiveWebContents());
  visible_area_button_->SetEnabled(can_capture);
  full_page_button_->SetEnabled(can_capture);
}

ScreenshotBubbleView::~ScreenshotBubbleView() = default;

content::WebContents* ScreenshotBubbleView::GetActiveWebContents() {
  return browser_ ? browser_->tab_strip_model()->GetActiveWebContents()
                  : nullptr;
}

void ScreenshotBubbleView::OnVisibleAreaPressed() {
  if (!controller_) {
    return;
  }
  // Stash before closing — the close may destroy `this` synchronously on some
  // platforms.
  auto* controller = controller_.get();
  auto* wc = GetActiveWebContents();
  GetWidget()->CloseWithReason(views::Widget::ClosedReason::kUnspecified);
  controller->CaptureVisibleArea(wc, base::DoNothing());
}

void ScreenshotBubbleView::OnFullPagePressed() {
  if (!controller_) {
    return;
  }
  auto* controller = controller_.get();
  auto* wc = GetActiveWebContents();
  GetWidget()->CloseWithReason(views::Widget::ClosedReason::kUnspecified);
  controller->CaptureFullPage(wc, base::DoNothing());
}

BEGIN_METADATA(ScreenshotBubbleView)
END_METADATA

}  // namespace brave
