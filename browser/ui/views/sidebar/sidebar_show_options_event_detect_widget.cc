/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_show_options_event_detect_widget.h"

#include <utility>

#include "base/memory/raw_ref.h"
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
  explicit ContentsView(SidebarShowOptionsEventDetectWidget::Delegate& delegate)
      : delegate_(delegate) {}

  ~ContentsView() override = default;

  ContentsView(const ContentsView&) = delete;
  ContentsView& operator=(const ContentsView&) = delete;

  void OnMouseEntered(const ui::MouseEvent& event) override {
    delegate_->ShowSidebarControlView();
  }

 private:
  raw_ref<SidebarShowOptionsEventDetectWidget::Delegate> delegate_;
};

SidebarShowOptionsEventDetectWidget::SidebarShowOptionsEventDetectWidget(
    BraveBrowserView& browser_view,
    Delegate& delegate)
    : browser_view_(browser_view), delegate_(delegate) {
  observation_.Observe(browser_view_->contents_container());
  widget_ = CreateWidget(delegate);

#if defined(USE_AURA)
  widget_->GetNativeView()->SetProperty(views::kHostViewKey,
                                        browser_view_->sidebar_host_view());
#endif
}

SidebarShowOptionsEventDetectWidget::~SidebarShowOptionsEventDetectWidget() {
  contents_view_ = nullptr;
}
void SidebarShowOptionsEventDetectWidget::Show() {
  DCHECK(widget_);
  AdjustWidgetBounds();
  widget_->ShowInactive();
}

void SidebarShowOptionsEventDetectWidget::Hide() {
  DCHECK(widget_);
  widget_->Hide();
}

void SidebarShowOptionsEventDetectWidget::SetSidebarOnLeft(
    bool sidebar_on_left) {
  sidebar_on_left_ = sidebar_on_left;
  AdjustWidgetBounds();
}

std::unique_ptr<views::Widget>
SidebarShowOptionsEventDetectWidget::CreateWidget(Delegate& delegate) {
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

void SidebarShowOptionsEventDetectWidget::OnViewIsDeleting(
    views::View* observed_view) {
  DCHECK(observation_.IsObservingSource(observed_view));
  observation_.Reset();
}

void SidebarShowOptionsEventDetectWidget::AdjustWidgetBounds() {
  // Convert contents container's rect into widget's coordinate
  // to use it as a detect widget's bounds as detect widget is parented
  // to browser widget.
  auto rect = browser_view_->contents_container()->GetLocalBounds();
  auto point = rect.origin();
  views::View::ConvertPointToTarget(browser_view_->contents_container(),
                                    browser_view_->GetWidget()->GetRootView(),
                                    &point);
  rect.set_origin(point);
  constexpr int kWidgetNarrowWidth = 7;
  if (!sidebar_on_left_) {
    rect.set_x(rect.right() - kWidgetNarrowWidth);
  }
  rect.set_width(kWidgetNarrowWidth);

  contents_view_->SetPreferredSize(rect.size());

#if BUILDFLAG(IS_MAC)
  // On Mac, we can't set empty bounds for the widget.
  if (rect.IsEmpty())
    rect.set_size({kWidgetNarrowWidth, 1});
#endif
  widget_->SetBounds(rect);
}
