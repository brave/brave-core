/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/pref_names.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"

#if defined(USE_AURA)
#include "ui/views/view_constants_aura.h"
#endif

// static
VerticalTabStripWidgetDelegateView* VerticalTabStripWidgetDelegateView::Create(
    BrowserView* browser_view,
    views::View* host_view) {
  DCHECK(browser_view->GetWidget());

  auto* delegate_view =
      new VerticalTabStripWidgetDelegateView(browser_view, host_view);
  views::Widget::InitParams params;
  params.delegate = delegate_view;

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

  return delegate_view;
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
  if (!host_)
    return;

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
  if (!host_)
    return;

  auto* widget = GetWidget();
  if (!widget)
    return;

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

#if BUILDFLAG(IS_MAC)
  UpdateClip();
#endif
}

void VerticalTabStripWidgetDelegateView::OnWidgetDestroying(
    views::Widget* widget) {
  widget_observation_.Reset();
}

#if BUILDFLAG(IS_MAC)
void VerticalTabStripWidgetDelegateView::UpdateClip() {
  // On mac, child window can be drawn out of parent window. We should clip
  // the border line and corner radius manually.
  SkPath path;
  const bool is_vertical_tab_left_most =
      !static_cast<BraveBrowserView*>(browser_view_)->IsSidebarVisible() ||
      !browser_view_->browser()->profile()->GetPrefs()->GetBoolean(
          prefs::kSidePanelHorizontalAlignment);

  if (is_vertical_tab_left_most) {
    // We should clip the bottom-left corner too.
    // The corner radius value refers to the that of menu widget. Looks fit for
    // us.
    // https://github.com/chromium/chromium/blob/371d67fd9c7db16c32f22e3ba247a07aa5e81487/ui/views/controls/menu/menu_config_mac.mm#L35
    constexpr int kCornerRadius = 8;
    path.moveTo(1, 0);
    path.lineTo(width(), 0);
    path.lineTo(width(), height() - 1);
    path.lineTo(1 + kCornerRadius, height() - 1);
    path.rArcTo(kCornerRadius, kCornerRadius, 0, SkPath::kSmall_ArcSize,
                SkPathDirection::kCW, -kCornerRadius, -kCornerRadius);
    path.close();
  } else {
    path.lineTo(width(), 0);
    path.lineTo(width(), height() - 1);
    path.lineTo(0, height() - 1);
    path.close();
  }
  SetClipPath(path);
}
#endif

BEGIN_METADATA(VerticalTabStripWidgetDelegateView, views::View)
END_METADATA
