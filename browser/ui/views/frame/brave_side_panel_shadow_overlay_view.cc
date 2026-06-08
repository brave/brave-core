/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_side_panel_shadow_overlay_view.h"

#include <memory>

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/view_shadow.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"

namespace {

// Transparent margin added around the panel so the blurred shadow has room to
// render before being clipped to the browser view. The visible blur extends
// roughly `2 * blur_radius` px; a slightly larger margin is harmless because it
// is clipped away by MasksToBounds.
constexpr int kShadowMargin = 8;

}  // namespace

BraveSidePanelShadowOverlayView::BraveSidePanelShadowOverlayView(
    BrowserView& browser_view)
    : browser_view_(browser_view) {
  // Purely decorative; never intercept events.
  SetCanProcessEventsWithinSubtree(false);

  // Paint to a clipped, non-opaque layer. MasksToBounds, combined with clamping
  // our bounds to the browser view in SyncToSidePanel(), is what guarantees the
  // shadow is never drawn outside the browser view (e.g. past the window edge
  // during the slide animation).
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  layer()->SetMasksToBounds(true);
  SetVisible(false);

  auto* panel = browser_view_->side_panel();
  CHECK(panel);
  panel_observation_.Observe(panel);

  panel_shape_ = AddChildView(std::make_unique<views::View>());

  const int radius = views::LayoutProvider::Get()->GetCornerRadiusMetric(
      views::ShapeContextTokensOverride::kRoundedCornersBorderRadius);
  shadow_ = BraveContentsViewUtil::CreateShadow(panel_shape_,
                                                gfx::RoundedCornersF(radius));
  shadow_->SetVisible(false);
}

BraveSidePanelShadowOverlayView::~BraveSidePanelShadowOverlayView() = default;

void BraveSidePanelShadowOverlayView::UpdateShadowVisibility() {
  SyncToSidePanel();
}

void BraveSidePanelShadowOverlayView::OnViewBoundsChanged(
    views::View* /*observed_view*/) {
  SyncToSidePanel();
}

void BraveSidePanelShadowOverlayView::OnViewVisibilityChanged(
    views::View* /*observed_view*/,
    views::View* /*starting_view*/,
    bool /*is_visible*/) {
  SyncToSidePanel();
}

void BraveSidePanelShadowOverlayView::OnViewIsDeleting(
    views::View* observed_view) {
  auto* panel = panel_observation_.GetSource();
  CHECK_EQ(panel, observed_view);
  panel_observation_.Reset();
}

void BraveSidePanelShadowOverlayView::SyncToSidePanel() {
  auto* panel = panel_observation_.GetSource();
  const bool rounded_corners =
      BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
          browser_view_->browser());

  if (!panel || !panel->GetVisible() || !rounded_corners) {
    shadow_->SetVisible(false);
    SetVisible(false);
    return;
  }

  // `panel->bounds()` is in browser-view coordinates; this overlay is also a
  // child of the browser view, so they share a coordinate space.
  const gfx::Rect panel_bounds = panel->bounds();

  // Our bounds: the panel plus a margin for the blur, clamped to the browser
  // view. Clamping + MasksToBounds clips any part of the shadow that would fall
  // outside the browser view (e.g. on the window edge during the slide).
  gfx::Rect overlay_bounds = panel_bounds;
  overlay_bounds.Inset(-kShadowMargin);
  overlay_bounds.Intersect(browser_view_->GetLocalBounds());
  SetBoundsRect(overlay_bounds);

  // Position the shadow shape over the panel's *visible* rounded area, in our
  // local coordinates. SidePanel::UpdateBorder() pads the panel - a bottom and
  // outer-side margin plus a header-height top inset - so the rounded content
  // is smaller than panel->bounds(). Shrink the shape by those same insets so
  // the shadow hugs the visible edges rather than the raw bounds (otherwise the
  // padded sides look offset/thicker). The top inset is dropped because the
  // header fills it and shares the panel's rounded top, so the shadow should
  // still wrap the very top edge.
  // The blur around the shape may extend past our (clamped) bounds; that
  // overflow is exactly what MasksToBounds clips so the shadow never leaves the
  // browser view.
  gfx::Rect shape_bounds = panel_bounds;
  shape_bounds.Offset(-overlay_bounds.x(), -overlay_bounds.y());
  shape_bounds.Inset(panel->GetInsets().set_top(0));
  panel_shape_->SetBoundsRect(shape_bounds);

  SetVisible(true);
  shadow_->SetVisible(true);
}

BEGIN_METADATA(BraveSidePanelShadowOverlayView)
END_METADATA
