/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_root_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/views/frame/browser_root_view.h"
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

class VerticalTabStripWidget : public ThemeCopyingWidget {
 public:
  VerticalTabStripWidget(BrowserView* browser_view, views::Widget* widget)
      : ThemeCopyingWidget(widget), browser_view_(browser_view) {}
  ~VerticalTabStripWidget() override = default;

  // ThemeCopyingWidget:
  views::internal::RootView* CreateRootView() override {
    return new VerticalTabStripRootView(browser_view_, this);
  }

 private:
  raw_ptr<BrowserView> browser_view_;
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

  auto widget = std::make_unique<VerticalTabStripWidget>(
      browser_view, browser_view->GetWidget());
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

  DCHECK(fullscreen_observation_.IsObserving())
      << "We didn't start to observe FullscreenController from BrowserList's "
         "callback";
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
  widget_observation_.AddObservation(host_->GetWidget());

  // At this point, Browser hasn't finished its initialization. In order to
  // access some of its member, we should observe BrowserList.
  DCHECK(base::ranges::find(*BrowserList::GetInstance(),
                            browser_view_->browser()) ==
         BrowserList::GetInstance()->end())
      << "Browser shouldn't be added at this point.";
  BrowserList::AddObserver(this);

  ChildPreferredSizeChanged(region_view_);
}

void VerticalTabStripWidgetDelegateView::AddedToWidget() {
  widget_observation_.AddObservation(GetWidget());
}

void VerticalTabStripWidgetDelegateView::ChildPreferredSizeChanged(
    views::View* child) {
  if (!host_) {
    return;
  }

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
  if (!widget || widget->IsVisible() == observed_view->GetVisible()) {
    return;
  }

  if (observed_view->GetVisible()) {
    widget->Show();
  } else {
    widget->Hide();
  }
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

void VerticalTabStripWidgetDelegateView::OnBrowserAdded(Browser* browser) {
  if (browser != browser_view_->browser()) {
    return;
  }

  auto* exclusive_access_manager =
      browser_view_->browser()->exclusive_access_manager();
  DCHECK(exclusive_access_manager);

  auto* fullscreen_controller =
      exclusive_access_manager->fullscreen_controller();
  DCHECK(fullscreen_controller);
  fullscreen_observation_.Observe(fullscreen_controller);

  BrowserList::RemoveObserver(this);
}

void VerticalTabStripWidgetDelegateView::OnWidgetVisibilityChanged(
    views::Widget* widget,
    bool visible) {
  if (widget == GetWidget()) {
    if (!tabs::utils::ShouldShowVerticalTabs(browser_view_->browser()) &&
        visible) {
      // This happens when restoring browser window. The upstream implementation
      // make child widgets visible regardless of their previous visibility.
      // https://github.com/brave/brave-browser/issues/29917
      widget->Hide();
    }
  }
}

void VerticalTabStripWidgetDelegateView::OnWidgetBoundsChanged(
    views::Widget* widget,
    const gfx::Rect& new_bounds) {
  if (widget == GetWidget()) {
    return;
  }

  // The parent widget could be resized because fullscreen status changed.
  // Try resetting preferred size.
  ChildPreferredSizeChanged(region_view_);
}

void VerticalTabStripWidgetDelegateView::UpdateWidgetBounds() {
  if (!host_) {
    return;
  }

  auto* widget = GetWidget();
  if (!widget) {
    return;
  }

  // Convert coordinate system based on Browser's widget.
  gfx::Rect widget_bounds = host_->ConvertRectToWidget(host_->GetLocalBounds());
  widget_bounds.set_width(region_view_->GetPreferredSize().width());
  if (widget_bounds.IsEmpty()) {
    widget->Hide();
    return;
  }

  DCHECK(tabs::utils::ShouldShowVerticalTabs(browser_view_->browser()));

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

  if (need_to_call_layout) {
    Layout();
  }

#if BUILDFLAG(IS_MAC)
  UpdateClip();
#endif
}

void VerticalTabStripWidgetDelegateView::OnWidgetDestroying(
    views::Widget* widget) {
  widget_observation_.RemoveObservation(widget);
}

void VerticalTabStripWidgetDelegateView::OnFullscreenStateChanged() {
  ChildPreferredSizeChanged(region_view_);
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
