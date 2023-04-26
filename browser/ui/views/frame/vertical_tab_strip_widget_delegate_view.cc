/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/theme_copying_widget.h"
#include "chrome/common/pref_names.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view_utils.h"

#if defined(USE_AURA)
#include "ui/views/view_constants_aura.h"
#endif

namespace {

class VerticalTabStripRootView : public views::internal::RootView {
 public:
  METADATA_HEADER(VerticalTabStripRootView);

  using RootView::RootView;
  ~VerticalTabStripRootView() override = default;

  // views::internal::RootView:
  bool OnMousePressed(const ui::MouseEvent& event) override {
#if defined(USE_AURA)
    const bool result = RootView::OnMousePressed(event);
    auto* focus_manager = GetFocusManager();
    DCHECK(focus_manager);

    // When vertical tab strip area is clicked, shortcut handling process
    // could get broken on Windows. There are 2 paths where shortcut is handled.
    // One is BrowserView::AcceleratorPressed(), and the other is
    // BrowserView::PreHandleKeyboardEvent(). When web view has focus, the
    // first doesn't deal with it and the latter is responsible for the
    // shortcuts. when users click the vertical tab strip area with web view
    // focused, both path don't handle it. This is because focused view state of
    // views/ framework and focused native window state of Aura is out of sync.
    // So as a workaround, resets the focused view state so that shortcuts can
    // be handled properly. This shouldn't change the actually focused view, and
    // is just reset the status.
    // https://github.com/brave/brave-browser/issues/28090
    // https://github.com/brave/brave-browser/issues/27812
    if (auto* focused_view = focus_manager->GetFocusedView();
        focused_view && views::IsViewClass<views::WebView>(focused_view)) {
      focus_manager->ClearFocus();
      focus_manager->RestoreFocusedView();
    }

    return result;
#else
    // On Mac, the parent widget doesn't get activated in this case. Then
    // shortcut handling could malfunction. So activate it.
    // https://github.com/brave/brave-browser/issues/29993
    auto* widget = GetWidget();
    DCHECK(widget);
    widget = widget->GetTopLevelWidget();
    widget->Activate();

    return RootView::OnMousePressed(event);
#endif
  }
};

BEGIN_METADATA(VerticalTabStripRootView, views::internal::RootView)
END_METADATA

class VerticalTabStripWidget : public ThemeCopyingWidget {
 public:
  using ThemeCopyingWidget::ThemeCopyingWidget;
  ~VerticalTabStripWidget() override = default;

  // ThemeCopyingWidget:
  views::internal::RootView* CreateRootView() override {
    return new VerticalTabStripRootView(this);
  }
};

}  // namespace

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

  auto widget =
      std::make_unique<VerticalTabStripWidget>(browser_view->GetWidget());
  widget->Init(std::move(params));
#if defined(USE_AURA)
  widget->GetNativeView()->SetProperty(views::kHostViewKey, host_view);
#endif
  widget->Show();
  widget.release();

  return delegate_view;
}

VerticalTabStripWidgetDelegateView::~VerticalTabStripWidgetDelegateView() {
  // Child views will be deleted after this. Marks `region_view_` nullptr
  // so that they dont' access the `region_view_` via this view.
  region_view_ = nullptr;
}

VerticalTabStripWidgetDelegateView::VerticalTabStripWidgetDelegateView(
    BrowserView* browser_view,
    views::View* host)
    : browser_view_(browser_view),
      host_(host),
      region_view_(AddChildView(std::make_unique<VerticalTabStripRegionView>(
          browser_view_,
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
  if (auto new_host_size = region_view_->GetMinimumSize();
      new_host_size != host_->GetPreferredSize()) {
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
  // The parent widget could be resized because fullscreen status changed.
  // Try resetting preferred size.
  ChildPreferredSizeChanged(region_view_);
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

  auto insets = host_->GetInsets();
  widget_bounds.set_width(widget_bounds.width() + insets.width());
  if (GetInsets() != insets) {
    SetBorder(insets.IsEmpty() ? nullptr : views::CreateEmptyBorder(insets));
  }

  const bool need_to_call_layout =
      widget->GetWindowBoundsInScreen().size() != widget_bounds.size();
  widget->SetBounds(widget_bounds);

  if (!widget->IsVisible()) {
    widget->Show();
  }

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
  // The corner radius value refers to the that of menu widget. Looks fit for
  // us.
  // https://github.com/chromium/chromium/blob/371d67fd9c7db16c32f22e3ba247a07aa5e81487/ui/views/controls/menu/menu_config_mac.mm#L35
  SkPath path;
  constexpr int kCornerRadius = 8;
  path.moveTo(0, 0);
  path.lineTo(width(), 0);
  path.lineTo(width(), height() - 1);
  path.lineTo(0 + kCornerRadius, height() - 1);
  path.rArcTo(kCornerRadius, kCornerRadius, 0, SkPath::kSmall_ArcSize,
              SkPathDirection::kCW, -kCornerRadius, -kCornerRadius);
  path.close();
  SetClipPath(path);
}
#endif

BEGIN_METADATA(VerticalTabStripWidgetDelegateView, views::View)
END_METADATA
