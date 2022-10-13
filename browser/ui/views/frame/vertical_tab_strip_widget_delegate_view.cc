/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

#if defined(USE_AURA)
#include "ui/views/view_constants_aura.h"
#endif

// static
void VerticalTabStripWidgetDelegateView::Create(BrowserView* browser_view,
                                                views::View* host_view) {
  DCHECK(browser_view->GetWidget());

  views::Widget::InitParams params;
  params.delegate =
      new VerticalTabStripWidgetDelegateView(browser_view, host_view);
  params.parent = browser_view->GetWidget()->GetNativeView();
  params.type = views::Widget::InitParams::TYPE_CONTROL;
  // We need this to pass the key events to the top level widget. i.e. we should
  // not get focus.
  params.activatable = views::Widget::InitParams::Activatable::kNo;

  auto widget = std::make_unique<views::Widget>();
  widget->Init(std::move(params));
#if defined(USE_AURA)
  widget->GetNativeView()->SetProperty(views::kHostViewKey, host_view);
#endif
  widget->Show();
  widget.release();
}

VerticalTabStripWidgetDelegateView::~VerticalTabStripWidgetDelegateView() =
    default;

VerticalTabStripWidgetDelegateView::VerticalTabStripWidgetDelegateView(
    BrowserView* browser_view,
    views::View* host)
    : browser_view_(browser_view),
      host_(host),
      region_view_(AddChildView(std::make_unique<VerticalTabStripRegionView>(
          browser_view_->browser(),
          browser_view_->tab_strip_region_view()))) {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  host_view_observation_.Observe(host_);
  widget_observation_.Observe(host_->GetWidget());

  ChildPreferredSizeChanged(region_view_);
}

void VerticalTabStripWidgetDelegateView::ChildPreferredSizeChanged(
    views::View* child) {
  // Setting minimum size for |host_| so that we can overlay vertical tabs over
  // the web view.
  auto new_host_size = region_view_->GetMinimumSize();
  if (new_host_size != host_->GetPreferredSize()) {
    host_->SetPreferredSize(new_host_size);
    return;
  }

  // Layout widget manually because host won't trigger layout.
  UpdateWidgetBounds();
}

void VerticalTabStripWidgetDelegateView::OnViewVisibilityChanged(
    views::View* observed_view,
    views::View* starting_view) {
  auto* widget = GetWidget();
  if (!widget || widget->IsVisible() == observed_view->GetVisible())
    return;

  if (observed_view->GetVisible())
    widget->Show();
  else
    widget->Hide();
}

void VerticalTabStripWidgetDelegateView::OnViewBoundsChanged(
    views::View* observed_view) {
  UpdateWidgetBounds();
}

void VerticalTabStripWidgetDelegateView::OnViewIsDeleting(
    views::View* observed_view) {
  host_view_observation_.Reset();
  host_ = nullptr;
}

void VerticalTabStripWidgetDelegateView::OnWidgetBoundsChanged(
    views::Widget* widget,
    const gfx::Rect& new_bounds) {
  UpdateWidgetBounds();
}

void VerticalTabStripWidgetDelegateView::UpdateWidgetBounds() {
  auto* widget = GetWidget();
  DCHECK(widget);
  // Convert coordinate system based on Browser's widget.
  gfx::Rect widget_bounds = host_->ConvertRectToWidget(host_->GetLocalBounds());
  widget_bounds.set_width(region_view_->GetPreferredSize().width());
  if (widget_bounds.IsEmpty()) {
    widget->Hide();
    return;
  }

  if (!widget->IsVisible())
    widget->Show();

  const bool need_to_call_layout =
      widget->GetWindowBoundsInScreen().size() != widget_bounds.size();
  widget->SetBounds(widget_bounds);

  if (need_to_call_layout)
    Layout();
}

void VerticalTabStripWidgetDelegateView::OnWidgetDestroying(
    views::Widget* widget) {
  widget_observation_.Reset();
}

BEGIN_METADATA(VerticalTabStripWidgetDelegateView, views::View)
END_METADATA
