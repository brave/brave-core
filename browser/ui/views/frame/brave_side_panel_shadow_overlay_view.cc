/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_side_panel_shadow_overlay_view.h"

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/animation/browser_animation_controller.h"
#include "chrome/browser/ui/views/animations/side_panel_animations.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/shadow_frame_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/views/layout/layout_provider.h"

namespace {

// elevation=4 → ambient shadow: blur=8px, no y-offset (centered glow).
// Key shadow is zeroed out to keep the shadow symmetric.
constexpr int kShadowElevation = 8;
constexpr ShadowFrameView::ShadowAlpha kShadowAlpha{
    .light_key = 0.0,
    .light_ambient = 0.14,
    .dark_key = 0.0,
    .dark_ambient = 0.25,
};

}  // namespace

BraveSidePanelShadowOverlayView::BraveSidePanelShadowOverlayView(
    BrowserView& browser_view)
    : browser_view_(browser_view) {
  SetCanProcessEventsWithinSubtree(false);

  shadow_box_ = AddChildView(
      std::make_unique<ShadowFrameView>(kShadowElevation, kShadowAlpha));
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
  SetVisible(false);

  animation_subscription_ =
      BrowserAnimationController::From(browser_view_->browser())
          ->Subscribe(
              SidePanelAnimations::kSidePanel,
              base::BindRepeating(
                  &BraveSidePanelShadowOverlayView::OnAnimationProgressed,
                  base::Unretained(this)));
}

BraveSidePanelShadowOverlayView::~BraveSidePanelShadowOverlayView() {
  if (observed_panel_) {
    observed_panel_->RemoveObserver(this);
    observed_panel_ = nullptr;
  }
}

void BraveSidePanelShadowOverlayView::UpdateShadowVisibility() {
  SyncToSidePanel();
}

void BraveSidePanelShadowOverlayView::AddedToWidget() {
  shadow_box_->SetShadowCornerRadius(
      GetLayoutProvider()->GetCornerRadiusMetric(views::Emphasis::kHigh));

  if (auto* panel = browser_view_->side_panel()) {
    observed_panel_ = panel;
    observed_panel_->AddObserver(this);
  }
}

void BraveSidePanelShadowOverlayView::OnViewBoundsChanged(
    views::View* /*observed_view*/) {
  SyncToSidePanel();
}

void BraveSidePanelShadowOverlayView::OnViewVisibilityChanged(
    views::View* /*observed_view*/,
    views::View* /*starting_view*/,
    bool /*visible*/) {
  SyncToSidePanel();
}

void BraveSidePanelShadowOverlayView::OnViewIsDeleting(
    views::View* observed_view) {
  observed_view->RemoveObserver(this);
  if (observed_view == observed_panel_) {
    observed_panel_ = nullptr;
  }
}

void BraveSidePanelShadowOverlayView::OnAnimationProgressed(
    const BrowserAnimationController* /*controller*/,
    BrowserAnimationUpdate /*status*/) {
  SyncToSidePanel();
}

void BraveSidePanelShadowOverlayView::SyncToSidePanel() {
  auto* panel = browser_view_->side_panel();

  const bool rounded_corners =
      BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
          browser_view_->browser());

  if (!panel || !panel->GetVisible() || !rounded_corners) {
    shadow_box_->SetShadowVisible(false);
    SetVisible(false);
    return;
  }

  // Drive shadow opacity from the animation sequence for a smooth fade.
  // Falls back to 1.0 when fully open, 0.0 otherwise (e.g. no animation).
  const auto anim_value =
      BrowserAnimationController::From(browser_view_->browser())
          ->GetCurrentValue(SidePanelAnimations::kSidePanel,
                            SidePanelAnimations::kMainAreaShadow);
  const double shadow_opacity = anim_value.value_or(
      panel->state() == SidePanel::State::kOpen ? 1.0 : 0.0);

  // Expand the overlay beyond the panel bounds so the shadow layer has room
  // to render on all sides. shadow_box_ is then centered within the overlay
  // at exactly the panel size, so its shadow extends into the expanded area.
  // The spread is approximately 2 * elevation per side (4 * elevation total).
  constexpr int kSpread = kShadowElevation * 2;
  gfx::Rect panel_bounds = panel->bounds();
  gfx::Rect overlay_bounds = panel_bounds;
  overlay_bounds.Inset(-kSpread);
  SetBoundsRect(overlay_bounds);
  shadow_box_->SetBoundsRect(
      gfx::Rect(kSpread, kSpread, panel_bounds.width(), panel_bounds.height()));

  SetVisible(true);
  shadow_box_->SetShadowVisible(true);
  shadow_box_->SetShadowOpacity(shadow_opacity);
}

BEGIN_METADATA(BraveSidePanelShadowOverlayView)
END_METADATA
