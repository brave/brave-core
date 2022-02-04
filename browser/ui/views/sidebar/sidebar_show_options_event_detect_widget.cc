/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_show_options_event_detect_widget.h"

#include <utility>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#include "ui/views/view_constants_aura.h"
#endif

class SidebarShowOptionsEventDetectWidget::ContentsView : public views::View {
 public:
  explicit ContentsView(SidebarShowOptionsEventDetectWidget::Delegate* delegate)
      : delegate_(delegate) {}

  ~ContentsView() override = default;

  ContentsView(const ContentsView&) = delete;
  ContentsView& operator=(const ContentsView&) = delete;

  void OnMouseEntered(const ui::MouseEvent& event) override {
    delegate_->ShowSidebar();
  }

 private:
  SidebarShowOptionsEventDetectWidget::Delegate* delegate_ = nullptr;
};

SidebarShowOptionsEventDetectWidget::SidebarShowOptionsEventDetectWidget(
    BraveBrowserView* browser_view,
    Delegate* delegate)
    : browser_view_(browser_view), delegate_(delegate) {
  DCHECK(browser_view_);
  DCHECK(delegate_);
  observation_.Observe(browser_view_->contents_container());
  widget_ = CreateWidget(delegate);

#if defined(USE_AURA)
  widget_->GetNativeView()->SetProperty(views::kHostViewKey,
                                        browser_view_->sidebar_host_view());
#endif
}

SidebarShowOptionsEventDetectWidget::~SidebarShowOptionsEventDetectWidget() =
    default;

void SidebarShowOptionsEventDetectWidget::Show() {
  DCHECK(widget_);
  widget_->Show();
  AdjustWidgetBounds();
}

void SidebarShowOptionsEventDetectWidget::Hide() {
  DCHECK(widget_);
  widget_->Hide();
}

std::unique_ptr<views::Widget>
SidebarShowOptionsEventDetectWidget::CreateWidget(Delegate* delegate) {
  std::unique_ptr<views::Widget> widget(new views::Widget);
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_CONTROL);
  params.delegate = this;
  params.name = "SidebarEventDetectWidget";
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.parent = browser_view_->GetWidget()->GetNativeView();
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.activatable = views::Widget::InitParams::Activatable::kNo;
  widget->Init(std::move(params));

  std::unique_ptr<ContentsView> contents_view =
      std::make_unique<ContentsView>(delegate);
  contents_view_ = contents_view.get();
  widget->SetContentsView(std::move(contents_view));
  return widget;
}

void SidebarShowOptionsEventDetectWidget::OnViewBoundsChanged(
    views::View* observed_view) {
  AdjustWidgetBounds();
}

void SidebarShowOptionsEventDetectWidget::AdjustWidgetBounds() {
  auto rect = browser_view_->contents_container()->bounds();
  constexpr int kWidgetNarrowWidth = 7;
  rect.set_width(kWidgetNarrowWidth);
  contents_view_->SetPreferredSize(rect.size());
  widget_->SetBounds(rect);
}
