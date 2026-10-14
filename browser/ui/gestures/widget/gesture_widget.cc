// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/widget/gesture_widget.h"

#include <utility>

#include "base/check_deref.h"
#include "brave/browser/ui/gestures/widget/gesture_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/widget/widget.h"

namespace gestures {

GestureWidget::GestureWidget(content::WebContents* web_contents)
    : web_contents_(CHECK_DEREF(web_contents)) {}

GestureWidget::~GestureWidget() = default;

void GestureWidget::Show() {
  if (!widget_) {
    widget_ = std::make_unique<views::Widget>();
    views::Widget::InitParams params(
        views::Widget::InitParams::CLIENT_OWNS_WIDGET,
        views::Widget::InitParams::TYPE_POPUP);
    params.z_order = ui::ZOrderLevel::kFloatingUIElement;
    params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
    params.accept_events = false;
    params.activatable = views::Widget::InitParams::Activatable::kNo;
    params.bounds = web_contents_->GetContainerBounds();
    params.context = web_contents_->GetTopLevelNativeWindow();
    widget_->Init(std::move(params));
    gesture_view_ = widget_->SetContentsView(std::make_unique<GestureView>());
  }
  widget_->SetBounds(web_contents_->GetContainerBounds());
  widget_->ShowInactive();
}

bool GestureWidget::IsVisible() const {
  return widget_ && widget_->IsVisible();
}

void GestureWidget::Hide() {
  if (widget_) {
    widget_->Hide();
  }
}

void GestureWidget::UpdateTrail(bool show_trail,
                                const GesturePath& path,
                                const std::u16string& label) {
  if (gesture_view_) {
    gesture_view_->SetTrail(show_trail, path, label);
  }
}

}  // namespace gestures
