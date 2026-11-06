/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/i18n/rtl.h"
#include "base/memory/raw_ref.h"
#include "brave/browser/ui/views/brave_layout_provider.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "chrome/browser/devtools/devtools_ui_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_resize_area.h"
#include "chrome/browser/ui/views/frame/multi_contents_view_delegate.h"
#include "components/split_tabs/split_tab_visual_data.h"
#include "components/tabs/public/tab_interface.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/resize_area.h"
#include "ui/views/controls/resize_area_delegate.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr int kWebPanelResizeAreaWidth =
    MultiContentsResizeArea::kHandleResizeAxisSize +
    MultiContentsResizeArea::kHandleResizeAxisPadding;

// Self-delegating because BraveMultiContentsView already inherits
// MultiContentsView's OnResize() for the split view divider.
class WebPanelResizeArea : public views::ResizeArea,
                           public views::ResizeAreaDelegate {
  METADATA_HEADER(WebPanelResizeArea, views::ResizeArea)
 public:
  explicit WebPanelResizeArea(BraveMultiContentsView& multi_contents_view)
      : ResizeArea(this), multi_contents_view_(multi_contents_view) {}
  ~WebPanelResizeArea() override = default;

 private:
  // views::ResizeAreaDelegate:
  void OnResize(int resize_amount, bool done_resizing) override {
    multi_contents_view_->OnWebPanelResize(resize_amount, done_resizing);
  }

  const raw_ref<BraveMultiContentsView> multi_contents_view_;
};

BEGIN_METADATA(WebPanelResizeArea)
END_METADATA

// Returns |corner_radii| with the corners that meet the split divider replaced
// by the smaller border radius. Which corners those are depends on |layout|:
// the divider is vertical for kSideBySide and horizontal for kStacked.
gfx::RoundedCornersF GetSplitContentsCornerRadii(
    const gfx::RoundedCornersF& corner_radii,
    split_tabs::SplitTabLayout layout,
    bool is_first,
    bool is_last) {
  if (corner_radii.IsEmpty()) {
    return corner_radii;
  }

  const float adjacent_edge_radius =
      views::LayoutProvider::Get()->GetCornerRadiusMetric(
          ExtendedShapeContextTokens::kRoundedCornersBorderRadius);

  gfx::RoundedCornersF split_radii = corner_radii;
  if (layout == split_tabs::SplitTabLayout::kSideBySide) {
    // In RTL, the first split view is on the right and the last is on the left.
    // Swap directions before setting the visual right and left radii.
    if (base::i18n::IsRTL()) {
      is_last = std::exchange(is_first, is_last);
    }

    if (!is_first) {
      split_radii.set_upper_left(adjacent_edge_radius);
      split_radii.set_lower_left(adjacent_edge_radius);
    }
    if (!is_last) {
      split_radii.set_upper_right(adjacent_edge_radius);
      split_radii.set_lower_right(adjacent_edge_radius);
    }
  } else {
    if (!is_first) {
      split_radii.set_upper_left(adjacent_edge_radius);
      split_radii.set_upper_right(adjacent_edge_radius);
    }
    if (!is_last) {
      split_radii.set_lower_left(adjacent_edge_radius);
      split_radii.set_lower_right(adjacent_edge_radius);
    }
  }

  return split_radii;
}

}  // namespace

// static
BraveMultiContentsView* BraveMultiContentsView::From(MultiContentsView* view) {
  CHECK(view);
  return static_cast<BraveMultiContentsView*>(view);
}

BraveMultiContentsView::~BraveMultiContentsView() = default;

void BraveMultiContentsView::UseContentsContainerViewForWebPanel() {
  if (!contents_container_view_for_web_panel_) {
    contents_container_view_for_web_panel_ =
        AddChildView(std::make_unique<BraveContentsContainerView>(
            browser_view_, /*for_web_panel*/ true));
    contents_container_view_for_web_panel_->SetVisible(false);

    web_panel_resize_area_ =
        AddChildView(std::make_unique<WebPanelResizeArea>(*this));
    web_panel_resize_area_->SetVisible(false);

    SetWebPanelWidth(SidePanelEntry::kSidePanelDefaultContentWidth);

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
    DevtoolsUIController::From(browser_view_->browser())
        ->MakeSureControllerExists(contents_container_view_for_web_panel_);
  }
}

void BraveMultiContentsView::SetWebPanelContents(
    content::WebContents* web_contents) {
  UseContentsContainerViewForWebPanel();

  contents_container_view_for_web_panel_->contents_view()->SetWebContents(
      web_contents);
  contents_container_view_for_web_panel_->SetVisible(web_contents);
  UpdateContentsBorderAndOverlay();
}

bool BraveMultiContentsView::IsWebPanelVisible() const {
  return contents_container_view_for_web_panel_ &&
         contents_container_view_for_web_panel_->GetVisible();
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

void BraveMultiContentsView::OnWebPanelResize(int resize_amount,
                                              bool done_resizing) {
  if (!web_panel_width_on_resize_start_) {
    // Not |web_panel_width_|: if the window shrank below it, the drag would
    // have to make up the difference before the panel moved at all.
    web_panel_width_on_resize_start_ =
        ClampWebPanelWidth(GetWebPanelWidth(), width());
  }

  const int proposed_width =
      *web_panel_width_on_resize_start_ +
      (web_panel_on_left_ ? resize_amount : -resize_amount);
  SetWebPanelWidth(ClampWebPanelWidth(proposed_width, width()));

  if (done_resizing) {
    web_panel_width_on_resize_start_.reset();
  }
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
      container_view->SetContentsCornerRadii(GetSplitContentsCornerRadii(
          corner_radii, GetSplitLayout(), is_first, is_last));
    } else {
      container_view->SetContentsCornerRadii(corner_radii);
    }
  }

  UpdateContentsBorderAndOverlay();
}

views::ProposedLayout BraveMultiContentsView::CalculateProposedLayout(
    const views::SizeBounds& size_bounds) const {
  if (!size_bounds.is_fully_bounded() ||
      !contents_container_view_for_web_panel_) {
    return MultiContentsView::CalculateProposedLayout(size_bounds);
  }

  const int host_width = size_bounds.width().value();
  const int host_height = size_bounds.height().value();

  // Negative to shrink to make room for web panel and its resize area.
  const int web_panel_width =
      ClampWebPanelWidth(GetWebPanelWidth(), host_width);
  const int resize_area_width = GetWebPanelResizeAreaWidth();
  const int reserved_width = web_panel_width + resize_area_width;
  views::SizeBounds new_size_bounds = size_bounds;
  new_size_bounds.Enlarge(-reserved_width, 0);
  views::ProposedLayout layouts =
      MultiContentsView::CalculateProposedLayout(new_size_bounds);

  if (web_panel_on_left_) {
    for (auto& layout : layouts.child_layouts) {
      // Move all other views to right to put web panel on left side.
      layout.bounds.Offset(reserved_width, 0);
    }
  }

  const int web_panel_x = web_panel_on_left_ ? 0 : host_width - web_panel_width;
  gfx::Rect web_panel_rect(web_panel_x, 0, web_panel_width, host_height);
  layouts.child_layouts.emplace_back(
      contents_container_view_for_web_panel_.get(),
      contents_container_view_for_web_panel_->GetVisible(), web_panel_rect);

  const int resize_area_x =
      web_panel_on_left_ ? web_panel_width : host_width - reserved_width;
  layouts.child_layouts.emplace_back(
      web_panel_resize_area_.get(), IsWebPanelVisible(),
      gfx::Rect(resize_area_x, 0, resize_area_width, host_height));

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

int BraveMultiContentsView::GetWebPanelResizeAreaWidth() const {
  return IsWebPanelVisible() ? kWebPanelResizeAreaWidth : 0;
}

int BraveMultiContentsView::ClampWebPanelWidth(int requested_width,
                                               int host_width) const {
  if (!IsWebPanelVisible()) {
    return 0;
  }

  // Layout clamps too, not just dragging: the window can shrink after a drag
  // settled on a width that no longer fits.
  const int max_width =
      std::max(kMinWebContentsSize,
               host_width - GetWebPanelResizeAreaWidth() - kMinWebContentsSize);
  return std::clamp(requested_width, kMinWebContentsSize, max_width);
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
