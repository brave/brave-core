// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/screenshot_button.h"

#include "base/functional/bind.h"
#include "brave/browser/ui/screenshot/screenshot_controller.h"
#include "brave/browser/ui/views/screenshot/screenshot_bubble_view.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/widget/widget.h"

ScreenshotButton::ScreenshotButton(Browser* browser)
    : ToolbarButton(base::BindRepeating(&ScreenshotButton::ButtonPressed,
                                        base::Unretained(this))),
      browser_(browser),
      controller_(std::make_unique<brave::ScreenshotController>(
          browser->profile(),
          base::BindRepeating(
              [](Browser* b) -> gfx::NativeWindow {
                auto* w = b->window();
                return w ? w->GetNativeWindow() : gfx::NativeWindow();
              },
              browser))) {
  SetVectorIcon(kLeoScreenshotIcon);
  SetTooltipText(
      l10n_util::GetStringUTF16(IDS_BRAVE_SCREENSHOT_TOOLBAR_BUTTON_TOOLTIP));
}

ScreenshotButton::~ScreenshotButton() {
  if (bubble_widget_) {
    bubble_widget_->RemoveObserver(this);
  }
}

void ScreenshotButton::ButtonPressed() {
  if (bubble_widget_) {
    bubble_widget_->CloseWithReason(views::Widget::ClosedReason::kUnspecified);
    return;
  }
  bubble_widget_ =
      brave::ScreenshotBubbleView::Show(browser_, this, controller_.get());
  if (bubble_widget_) {
    bubble_widget_->AddObserver(this);
  }
}

void ScreenshotButton::OnWidgetDestroying(views::Widget* widget) {
  if (widget == bubble_widget_) {
    bubble_widget_->RemoveObserver(this);
    bubble_widget_ = nullptr;
  }
}

BEGIN_METADATA(ScreenshotButton)
END_METADATA
