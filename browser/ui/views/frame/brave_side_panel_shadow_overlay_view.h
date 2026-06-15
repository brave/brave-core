/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_SIDE_PANEL_SHADOW_OVERLAY_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_SIDE_PANEL_SHADOW_OVERLAY_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

class BrowserView;
class ViewShadow;

// Renders a drop shadow around the side panel that never draws outside the
// browser view.
//
// A shadow layer attached to the side panel's own `kBelow` region would ride
// along with the sliding panel and leak outside the window during the
// open/close animation. Instead, this view is a sibling of the side panel in
// BraveBrowserView. It:
//   * tracks the panel's bounds by observing it,
//   * paints the shadow into its own layer that is clipped (MasksToBounds) and
//     clamped to the browser view, so nothing is ever drawn outside it, and
//   * is stacked just *below* the panel, so the panel covers the inner part of
//     the shadow - leaving only the outer glow visible.
class BraveSidePanelShadowOverlayView : public views::View,
                                        public views::ViewObserver {
  METADATA_HEADER(BraveSidePanelShadowOverlayView, views::View)

 public:
  explicit BraveSidePanelShadowOverlayView(BrowserView& browser_view);
  BraveSidePanelShadowOverlayView(const BraveSidePanelShadowOverlayView&) =
      delete;
  BraveSidePanelShadowOverlayView& operator=(
      const BraveSidePanelShadowOverlayView&) = delete;
  ~BraveSidePanelShadowOverlayView() override;

  // Call when the rounded-corners preference or panel state may have changed
  // without a corresponding panel bounds/visibility change.
  void UpdateShadowVisibility();

 private:
  // views::ViewObserver:
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewVisibilityChanged(views::View* observed_view,
                               views::View* starting_view,
                               bool is_visible) override;
  void OnViewIsDeleting(views::View* observed_view) override;

  // Syncs this view's bounds, child shape and visibility to the side panel's
  // current bounds, visibility and rounded-corners state.
  void SyncToSidePanel();

  const raw_ref<BrowserView> browser_view_;
  base::ScopedObservation<views::View, views::ViewObserver> panel_observation_{
      this};
  // Transparent child sized exactly to the side panel. `shadow_` renders the
  // drop shadow around its rounded-rect shape.
  raw_ptr<views::View> panel_shape_ = nullptr;
  std::unique_ptr<ViewShadow> shadow_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_SIDE_PANEL_SHADOW_OVERLAY_VIEW_H_
