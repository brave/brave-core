/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/i18n/rtl.h"
#include "base/memory/ptr_util.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_root_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/tabs/features.h"
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

#if BUILDFLAG(IS_MAC)
#include "third_party/skia/include/core/SkPathBuilder.h"
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

  // views::Widget:
  bool ShouldViewsStyleFollowWidgetActivation() const override {
    // Want to make view consider widget activation state.
    // Ex, some controls apply disabled state when its widget is inactive.
    // As this widget is created as not-activatable,
    // need to explicitely give true by overriding this method.
    // Default impl is "return CanActivate()". So we need this override.
    return true;
  }

 private:
  raw_ptr<BrowserView> browser_view_;
};

}  // namespace

// static
std::unique_ptr<views::Widget> VerticalTabStripWidgetDelegateView::Create(
    BrowserView* browser_view,
    views::View* host_view) {
  DCHECK(browser_view->GetWidget());
  CHECK(!base::FeatureList::IsEnabled(tabs::kBraveVerticalTabStripEmbedded));

  auto* delegate_view =
      new VerticalTabStripWidgetDelegateView(browser_view, host_view);
  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_CONTROL);
  params.delegate = delegate_view;

  params.parent = browser_view->GetWidget()->GetNativeView();
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
  return widget;
}

// static
std::unique_ptr<VerticalTabStripWidgetDelegateView>
VerticalTabStripWidgetDelegateView::CreateEmbeddedInBrowserView(
    BrowserView* browser_view,
    views::View* host_view) {
  CHECK(base::FeatureList::IsEnabled(tabs::kBraveVerticalTabStripEmbedded));
  return base::WrapUnique(new VerticalTabStripWidgetDelegateView(
      browser_view, host_view, /*embedded_in_browser_view=*/true));
}

VerticalTabStripWidgetDelegateView::~VerticalTabStripWidgetDelegateView() {
  // Child views will be deleted after this. Marks `region_view_` nullptr
  // so that they dont' access the `region_view_` via this view.
  region_view_ = nullptr;
}

VerticalTabStripWidgetDelegateView::VerticalTabStripWidgetDelegateView(
    BrowserView* browser_view,
    views::View* host,
    bool embedded_in_browser_view)
    : browser_view_(browser_view),
      host_(host),
      region_view_(
          AddChildView(std::make_unique<BraveVerticalTabStripRegionView>(
              browser_view_,
              views::AsViewClass<HorizontalTabStripRegionView>(
                  browser_view_->tab_strip_view())))),
      embedded_in_browser_view_(embedded_in_browser_view) {
  if (embedded_in_browser_view_) {
    // Needs layer to render this over the webview.
    SetPaintToLayer();
  }

  // As we follow user's choice for vertical tab alignment,
  // we don't need to mirror this view.
  SetMirrored(false);
  SetLayoutManager(std::make_unique<views::FillLayout>());

  host_view_observation_.Observe(host_);

  if (!embedded_in_browser_view_) {
    widget_observation_.AddObservation(host_->GetWidget());
  }

  ChildPreferredSizeChanged(region_view_);
}

void VerticalTabStripWidgetDelegateView::AddedToWidget() {
  if (embedded_in_browser_view_) {
    return;
  }
  widget_observation_.AddObservation(GetWidget());
}

void VerticalTabStripWidgetDelegateView::ChildPreferredSizeChanged(
    views::View* child) {
  if (!host_) {
    return;
  }

  // Setting minimum size for |host_| so that we can overlay vertical tabs over
  // the web view.
  host_->SetPreferredSize(region_view_->GetMinimumSize());

  // The position could be changed, so we should lay out again.
  host_->InvalidateLayout();

  // Lay out the widget manually in case the host doesn't arrange it.
  // Ex, expand on mouse hover.
  if (embedded_in_browser_view_) {
    UpdateVerticalTabBounds();
  } else {
    UpdateWidgetBounds();
  }
}

void VerticalTabStripWidgetDelegateView::OnViewVisibilityChanged(
    views::View* observed_view,
    views::View* starting_view,
    bool visible) {
  if (embedded_in_browser_view_) {
    UpdateVerticalTabBounds();
    return;
  }

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
  if (embedded_in_browser_view_) {
    UpdateVerticalTabBounds();
  } else {
    UpdateWidgetBounds();
  }
}

void VerticalTabStripWidgetDelegateView::OnViewIsDeleting(
    views::View* observed_view) {
  host_view_observation_.Reset();
  host_ = nullptr;
}

void VerticalTabStripWidgetDelegateView::OnWidgetVisibilityChanged(
    views::Widget* widget,
    bool visible) {
  CHECK(!embedded_in_browser_view_);

  if (widget == GetWidget()) {
    if (!tabs::utils::ShouldShowBraveVerticalTabs(browser_view_->browser()) &&
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
  if (embedded_in_browser_view_) {
    return;
  }

  if (widget == GetWidget()) {
    return;
  }

  // The parent widget could be resized because fullscreen status changed.
  // Try resetting preferred size.
  ChildPreferredSizeChanged(region_view_);
}

void VerticalTabStripWidgetDelegateView::UpdateWidgetBounds() {
  CHECK(!embedded_in_browser_view_);

  if (!host_) {
    return;
  }

  auto* widget = GetWidget();
  if (!widget) {
    return;
  }

  // Convert coordinate system based on Browser's widget.
  gfx::Rect host_bounds = host_->ConvertRectToWidget(host_->GetLocalBounds());
  gfx::Rect widget_bounds = host_bounds;
  auto insets = host_->GetInsets();
  widget_bounds.set_width(region_view_->GetPreferredSize().width() +
                          insets.width());
  if (!region_view_->GetVisible() || widget_bounds.IsEmpty() ||
      !tabs::utils::ShouldShowBraveVerticalTabs(browser_view_->browser())) {
    widget->Hide();
    return;
  }

  if (GetInsets() != insets) {
    SetBorder(insets.IsEmpty() ? nullptr : views::CreateEmptyBorder(insets));
  }

  if (tabs::utils::IsVerticalTabOnRight(browser_view_->browser())) {
    // TODO(sko) This feels like a little bit janky during animation.
    // Test if we can alleviate it.
    widget_bounds.set_x(host_bounds.right() - widget_bounds.width());
  }

  widget->SetBounds(widget_bounds);

  if (!widget->IsVisible()) {
    widget->Show();
  }

#if BUILDFLAG(IS_MAC)
  UpdateClip();
#endif
}

void VerticalTabStripWidgetDelegateView::UpdateVerticalTabBounds() {
  CHECK(embedded_in_browser_view_);

  if (!host_) {
    return;
  }

  CHECK(host_->GetInsets().width() == 0)
      << "No additional horizontal insets for embedded vertical tab strip";

  const gfx::Rect host_bounds = host_->bounds();
  gfx::Rect strip_bounds = host_bounds;
  strip_bounds.set_width(region_view_->GetPreferredSize().width());

  if (!region_view_->GetVisible() || strip_bounds.IsEmpty()) {
    SetBoundsRect(gfx::Rect());
    return;
  }

  const bool on_right =
      tabs::utils::IsVerticalTabOnRight(browser_view_->browser());
  if (on_right) {
    strip_bounds.set_x(host_bounds.right() - strip_bounds.width());
  }

  // RTL: when the strip is wider than the host, correct horizontal overflow so
  // the extra width still extends toward the web contents; flip the shift when
  // vertical tabs are on the right.
  if (base::i18n::IsRTL()) {
    auto width_difference = strip_bounds.width() - host_bounds.width();
    width_difference = std::max(width_difference, 0);
    if (on_right) {
      width_difference = -width_difference;
    }
    strip_bounds.set_x(strip_bounds.x() - (width_difference));
  }

  SetBoundsRect(strip_bounds);
}

void VerticalTabStripWidgetDelegateView::OnWidgetDestroying(
    views::Widget* widget) {
  CHECK(!embedded_in_browser_view_);
  widget_observation_.RemoveObservation(widget);
}

#if BUILDFLAG(IS_MAC)
void VerticalTabStripWidgetDelegateView::UpdateClip() {
  CHECK(!embedded_in_browser_view_);

  // On mac, child window can be drawn out of parent window. We should clip
  // the border line and corner radius manually.
  const float corner_radius = GetVerticalTabStripCornerRadiusMac();
  if (tabs::utils::IsVerticalTabOnRight(browser_view_->browser())) {
    SkPathBuilder path;
    path.moveTo(width(), 0);
    path.lineTo(0, 0);
    path.lineTo(0, height() - 1);
    path.lineTo(width() - corner_radius, height() - 1);
    path.rArcTo({corner_radius, corner_radius}, 0,
                SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCCW,
                {corner_radius, -corner_radius});
    path.close();
    SetClipPath(path.detach());
  } else {
    SkPathBuilder path;
    path.moveTo(0, 0);
    path.lineTo(width(), 0);
    path.lineTo(width(), height() - 1);
    path.lineTo(corner_radius, height() - 1);
    path.rArcTo({corner_radius, corner_radius}, 0,
                SkPathBuilder::kSmall_ArcSize, SkPathDirection::kCW,
                {-corner_radius, -corner_radius});
    path.close();
    SetClipPath(path.detach());
  }
}
#endif

BEGIN_METADATA(VerticalTabStripWidgetDelegateView)
END_METADATA
