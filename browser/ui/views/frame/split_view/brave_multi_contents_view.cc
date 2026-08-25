/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"

#include <utility>

#include "base/check.h"
#include "base/i18n/rtl.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "chrome/browser/devtools/devtools_ui_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_background_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_resize_area.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_delegate.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"

namespace {

// Returns |corner_radii| with the corners that meet the split divider replaced
// by the smaller border radius.
gfx::RoundedCornersF GetSplitContentsCornerRadii(
    const gfx::RoundedCornersF& corner_radii,
    bool is_first,
    bool is_last) {
  if (corner_radii.IsEmpty()) {
    return corner_radii;
  }

  // In RTL, the first split view is on the right and the last is on the left.
  // Swap directions before setting the visual right and left radii.
  if (base::i18n::IsRTL()) {
    is_last = std::exchange(is_first, is_last);
  }

  const float adjacent_edge_radius =
      views::LayoutProvider::Get()->GetCornerRadiusMetric(
          views::ShapeContextTokensOverride::kRoundedCornersBorderRadius);

  gfx::RoundedCornersF split_radii = corner_radii;
  if (!is_first) {
    split_radii.set_upper_left(adjacent_edge_radius);
    split_radii.set_lower_left(adjacent_edge_radius);
  }
  if (!is_last) {
    split_radii.set_upper_right(adjacent_edge_radius);
    split_radii.set_lower_right(adjacent_edge_radius);
  }

  return split_radii;
}

}  // namespace

// static
BraveMultiContentsView* BraveMultiContentsView::From(MultiContentsView* view) {
  CHECK(view);
  return static_cast<BraveMultiContentsView*>(view);
}

BraveMultiContentsView::BraveMultiContentsView(
    BrowserView* browser_view,
    std::unique_ptr<MultiContentsViewDelegate> delegate)
    : MultiContentsView(browser_view, std::move(delegate)) {
  // Use rounded corners margin as resize area's width.
  resize_area_->SetPreferredSize(
      gfx::Size(kRoundedCornersContentsViewMargin, 0));
}

BraveMultiContentsView::~BraveMultiContentsView() = default;

void BraveMultiContentsView::UseContentsContainerViewForWebPanel() {
  if (!contents_container_view_for_web_panel_) {
    contents_container_view_for_web_panel_ =
        AddChildView(std::make_unique<BraveContentsContainerView>(
            browser_view_, /*for_web_panel*/ true));
    contents_container_view_for_web_panel_->SetVisible(false);

    auto& view_map =
        container_focusable_map_[contents_container_view_for_web_panel_];
    auto* contents_view =
        contents_container_view_for_web_panel_->contents_view();
    view_map[contents_view->GetClassName()] = contents_view;

    contents_focused_subscriptions_.push_back(
        contents_container_view_for_web_panel_->contents_view()
            ->AddWebContentsFocusedCallback(base::BindRepeating(
                &BraveMultiContentsView::OnWebContentsFocused,
                base::Unretained(this))));
    browser_view_->browser()
        ->GetFeatures()
        .devtools_ui_controller()
        ->MakeSureControllerExists(contents_container_view_for_web_panel_);
  }
}

void BraveMultiContentsView::SetWebPanelContents(
    content::WebContents* web_contents) {
  CHECK(contents_container_view_for_web_panel_);
  contents_container_view_for_web_panel_->contents_view()->SetWebContents(
      web_contents);
  contents_container_view_for_web_panel_->SetVisible(web_contents);
  UpdateContentsBorderAndOverlay();
}

bool BraveMultiContentsView::IsWebPanelVisible() const {
  CHECK(contents_container_view_for_web_panel_);
  return contents_container_view_for_web_panel_->GetVisible();
}

views::View*
BraveMultiContentsView::GetWebPanelContentsViewForTesting()  // IN-TEST
    const {
  return contents_container_view_for_web_panel_
             ? contents_container_view_for_web_panel_->contents_view()
             : nullptr;
}

void BraveMultiContentsView::SetWebPanelWidth(int width) {
  web_panel_width_ = width;
  InvalidateLayout();
}

void BraveMultiContentsView::SetWebPanelOnLeft(bool left) {
  web_panel_on_left_ = left;
  InvalidateLayout();
}

void BraveMultiContentsView::OnShowActiveContentsDomainChanged() {
  UpdateContentsBorderAndOverlay();
}

void BraveMultiContentsView::UpdateContentsCornerRadii(
    const gfx::RoundedCornersF& corner_radii) {
  if (contents_container_view_for_web_panel_) {
    contents_container_view_for_web_panel_->SetContentsCornerRadii(
        corner_radii);
  }

  const bool is_in_split = IsInSplitView();
  const size_t view_count = contents_container_views_.size();
  for (size_t i = 0; i < view_count; ++i) {
    auto* container_view =
        BraveContentsContainerView::From(contents_container_views_[i]);
    if (is_in_split) {
      const bool is_first = i == 0;
      const bool is_last = i == view_count - 1;
      container_view->SetContentsCornerRadii(
          GetSplitContentsCornerRadii(corner_radii, is_first, is_last));
    } else {
      container_view->SetContentsCornerRadii(corner_radii);
    }
  }

  UpdateContentsBorderAndOverlay();
}

views::ProposedLayout BraveMultiContentsView::CalculateProposedLayout(
    const views::SizeBounds& size_bounds) const {
  if (!size_bounds.is_fully_bounded()) {
    return MultiContentsView::CalculateProposedLayout(size_bounds);
  }

  views::SizeBounds new_size_bounds = size_bounds;

  // Negative to shrink to make room for web panel.
  const int web_panel_width = GetWebPanelWidth();
  new_size_bounds.Enlarge(-web_panel_width, 0);
  views::ProposedLayout layouts =
      MultiContentsView::CalculateProposedLayout(new_size_bounds);

  // Always hide |background_view_|. Due to layered |background_view_|,
  // our custom border drawn by BraveContentsContainerView is not visible
  // because BraveContentsContainerView doesn't have layer and its parent
  // layer is behind the |background_view_|.
  // We can handle this by having BraveContentsContainerView's own layer but
  // it's resource waste because we don't need |background_view_|. We alreay
  // have CustomBackground that fills contents area with toolbar color.
  auto* background_view_layout = layouts.GetLayoutFor(background_view_.get());
  CHECK(background_view_layout);
  background_view_layout->visible = false;

  if (!contents_container_view_for_web_panel_) {
    return layouts;
  }

  if (web_panel_on_left_) {
    for (auto& layout : layouts.child_layouts) {
      // Move all other views to right to put web panel on left side.
      layout.bounds.Offset(web_panel_width, 0);
    }
  }

  int host_width = size_bounds.width().value();
  int host_height = size_bounds.height().value();
  const int web_panel_x = web_panel_on_left_ ? 0 : host_width - web_panel_width;
  gfx::Rect web_panel_rect(web_panel_x, 0, web_panel_width, host_height);
  layouts.child_layouts.emplace_back(
      contents_container_view_for_web_panel_.get(),
      contents_container_view_for_web_panel_->GetVisible(), web_panel_rect);

  layouts.host_size = gfx::Size(host_width, host_height);
  return layouts;
}

void BraveMultiContentsView::ResetResizeArea() {
  // Give same width on both contents view.
  // Pass true to make delegate save ratio in session service like resizing
  // complete.
  delegate_->ResizeWebContents(0.5, /*done_resizing=*/true);
}

void BraveMultiContentsView::UpdateContentsBorderAndOverlay() {
  if (!contents_container_view_for_web_panel_ ||
      !contents_container_view_for_web_panel_->GetVisible()) {
    MultiContentsView::UpdateContentsBorderAndOverlay();
    return;
  }

  if (!contents_container_view_for_web_panel_->IsActive()) {
    // Web panel is visible but inactive. Hide border of web panel.
    contents_container_view_for_web_panel_->UpdateBorderAndOverlay(
        /*is_in_split*/ false, /*is_active*/ false,
        /*is_highlighted*/ false);
    MultiContentsView::UpdateContentsBorderAndOverlay();
    return;
  }

  // When web panel is active, only it should have active border.
  contents_container_view_for_web_panel_->UpdateBorderAndOverlay(
      /*is_in_split*/ false, /*is_active*/ true,
      /*is_highlighted*/ false);

  for (ContentsContainerView* contents_container_view :
       contents_container_views_) {
    contents_container_view->UpdateBorderAndOverlay(IsInSplitView(),
                                                    /*is_active*/ false,
                                                    /*is_highlighted*/ false);
  }
}

void BraveMultiContentsView::OnWebContentsFocused(views::WebView* web_view) {
  // Early return if web panel is not used.
  if (!contents_container_view_for_web_panel_ ||
      !contents_container_view_for_web_panel_->GetVisible()) {
    MultiContentsView::OnWebContentsFocused(web_view);
    return;
  }

  // When a tab is detached, focus manager could make another web contents
  // focused. We don't need to make web contents' tab activated here. Next
  // active tab could be selected by tab strip model and its web contents will
  // get focused.
  if (!GetActiveContentsView()->web_contents()) {
    return;
  }

  // When tab is activated from Tab UI, we don't need to ask activate
  // |web_view|'s contents to TabStripModel again. It's not activated here when
  // activating tab by clicking contents.
  if (auto* tab =
          tabs::TabInterface::MaybeGetFromContents(web_view->web_contents());
      tab && tab->IsActivated()) {
    return;
  }

  // Base class only gives focus for inactive split tab because that inactive
  // split tab could get focused in upstream.
  // With web panel feature, other tabs also could get focused.
  // When web panel has focus, previously active split tab could get focus.
  // Also, web panel's tab also needs focus.
  // So, notify always.
  delegate_->WebContentsFocused(web_view->web_contents());
}

void BraveMultiContentsView::ExecuteOnEachVisibleContentsView(
    base::RepeatingCallback<void(ContentsWebView*)> callback) {
  if (contents_container_view_for_web_panel_ &&
      contents_container_view_for_web_panel_->GetVisible()) {
    callback.Run(contents_container_view_for_web_panel_->contents_view());
  }

  MultiContentsView::ExecuteOnEachVisibleContentsView(callback);
}

int BraveMultiContentsView::GetWebPanelWidth() const {
  if (!contents_container_view_for_web_panel_ ||
      !contents_container_view_for_web_panel_->GetVisible()) {
    return 0;
  }

  return web_panel_width_;
}

ContentsContainerView* BraveMultiContentsView::GetActiveContentsContainerView()
    const {
  if (is_web_panel_active_) {
    return contents_container_view_for_web_panel_;
  }

  return MultiContentsView::GetActiveContentsContainerView();
}

ContentsWebView* BraveMultiContentsView::GetActiveContentsView() const {
  if (is_web_panel_active_) {
    return contents_container_view_for_web_panel_->contents_view();
  }

  return MultiContentsView::GetActiveContentsView();
}

ContentsContainerView* BraveMultiContentsView::GetContentsContainerViewFor(
    content::WebContents* web_contents) const {
  if (is_web_panel_active_ &&
      contents_container_view_for_web_panel_->contents_view()->web_contents() ==
          web_contents) {
    return contents_container_view_for_web_panel_;
  }

  return MultiContentsView::GetContentsContainerViewFor(web_contents);
}

BEGIN_METADATA(BraveMultiContentsView)
END_METADATA
