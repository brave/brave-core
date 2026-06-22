// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/screenshot_button.h"

#include "base/functional/bind.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/browser/ui/views/toolbar/screenshot_bubble_view.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/bubble/bubble_dialog_model_host.h"
#include "ui/views/widget/widget.h"

ScreenshotButton::ScreenshotButton(
    BrowserWindowInterface* browser_window_interface)
    : ToolbarButton(base::BindRepeating(&ScreenshotButton::ButtonPressed,
                                        base::Unretained(this))),
      browser_window_interface_(browser_window_interface) {
  SetVectorIcon(kLeoScreenshotIcon);
  SetTooltipText(
      l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_TOOLBAR_BUTTON_TOOLTIP));
}

ScreenshotButton::~ScreenshotButton() {
  SetCallback(PressedCallback());
}

void ScreenshotButton::ButtonPressed() {
  if (bubble_widget_) {
    bubble_widget_->CloseWithReason(views::Widget::ClosedReason::kUnspecified);
    return;
  }

  auto* controller =
      browser_window_interface_->GetFeatures().screenshot_controller();
  auto host = screenshot::ShowScreenshotBubble(
      browser_window_interface_->GetActiveTabInterface()->GetContents(), this,
      controller);
  // The host is owned by the bubble.
  bubble_widget_ = views::BubbleDialogDelegate::CreateBubble(
      host.release(), base::BindOnce(&ScreenshotButton::OnBubbleClosing,
                                     weak_ptr_factory_.GetWeakPtr()));
  bubble_widget_->Show();
}

void ScreenshotButton::OnBubbleClosing(views::Widget::ClosedReason reason) {
  base::SingleThreadTaskRunner::GetCurrentDefault()->DeleteSoon(
      FROM_HERE, bubble_widget_.release());
}

BEGIN_METADATA(ScreenshotButton) END_METADATA
