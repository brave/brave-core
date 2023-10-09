/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/autofill/confirm_autocomplete_bubble_view.h"

#include "brave/browser/ui/autofill/confirm_autocomplete_bubble_controller.h"
#include "chrome/browser/ui/views/accessibility/theme_tracking_non_accessible_image_view.h"
#include "chrome/grit/theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

namespace autofill {

ConfirmAutocompleteBubbleView::ConfirmAutocompleteBubbleView(
    views::View* anchor_view,
    content::WebContents* web_contents,
    ConfirmAutocompleteBubbleController* controller)
    : LocationBarBubbleDelegateView(anchor_view, web_contents),
      controller_(controller) {
  DCHECK(controller_);
  SetButtonLabel(ui::DIALOG_BUTTON_OK, controller_->GetAcceptButtonText());
  SetButtonLabel(ui::DIALOG_BUTTON_CANCEL, controller_->GetDeclineButtonText());
  SetAcceptCallback(
      base::BindOnce(&ConfirmAutocompleteBubbleView::OnDialogAccepted,
                     base::Unretained(this)));

  SetShowCloseButton(true);
  set_fixed_width(views::LayoutProvider::Get()->GetDistanceMetric(
      views::DISTANCE_BUBBLE_PREFERRED_WIDTH));
}

void ConfirmAutocompleteBubbleView::Show(DisplayReason reason) {
  ShowForReason(reason);
}

void ConfirmAutocompleteBubbleView::Hide() {
  CloseBubble();

  // If `controller_` is null, WindowClosing() won't invoke OnBubbleClosed(), so
  // do that here. This will clear out `controller_`'s reference to `this`. Note
  // that WindowClosing() happens only after the _asynchronous_ Close() task
  // posted in CloseBubble() completes, but we need to fix references sooner.
  if (controller_) {
    auto* widget = GetWidget();
    DCHECK(widget);
    controller_->OnBubbleClosed(widget->closed_reason());
  }
  controller_ = nullptr;
}

void ConfirmAutocompleteBubbleView::AddedToWidget() {
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();

  GetBubbleFrameView()->SetHeaderView(
      std::make_unique<ThemeTrackingNonAccessibleImageView>(
          *bundle.GetImageSkiaNamed(IDR_SAVE_CARD),
          *bundle.GetImageSkiaNamed(IDR_SAVE_CARD_DARK),
          base::BindRepeating(&views::BubbleDialogDelegate::GetBackgroundColor,
                              base::Unretained(this))));
}

std::u16string ConfirmAutocompleteBubbleView::GetWindowTitle() const {
  return controller_ ? controller_->GetWindowTitle() : u"";
}

void ConfirmAutocompleteBubbleView::WindowClosing() {
  if (controller_) {
    auto* widget = GetWidget();
    DCHECK(widget);
    controller_->OnBubbleClosed(widget->closed_reason());
    controller_ = nullptr;
  }
}

ConfirmAutocompleteBubbleView::~ConfirmAutocompleteBubbleView() = default;

void ConfirmAutocompleteBubbleView::OnDialogAccepted() {
  if (controller_) {
    controller_->OnAcceptButton();
  }
}

}  // namespace autofill
