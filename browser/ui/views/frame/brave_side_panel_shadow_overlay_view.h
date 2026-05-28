/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_SIDE_PANEL_SHADOW_OVERLAY_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_SIDE_PANEL_SHADOW_OVERLAY_VIEW_H_

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/animation/browser_animation_types.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

class BrowserAnimationController;
class BrowserView;
class ShadowFrameView;

// A shadow overlay view that follows the SidePanel's bounds.
// Added as a sibling of side_panel_ in BraveBrowserView and rendered on top.
// Does not modify side_panel_'s own layer. Shadow opacity is driven by the
// SidePanelAnimations::kMainAreaShadow sequence for a smooth fade. Visible
// only when the panel is fully open and rounded corners are enabled.
class BraveSidePanelShadowOverlayView : public views::View,
                                        public views::ViewObserver {
  METADATA_HEADER(BraveSidePanelShadowOverlayView, views::View)

 public:
  explicit BraveSidePanelShadowOverlayView(BrowserView& browser_view);
  ~BraveSidePanelShadowOverlayView() override;

  BraveSidePanelShadowOverlayView(const BraveSidePanelShadowOverlayView&) =
      delete;
  BraveSidePanelShadowOverlayView& operator=(
      const BraveSidePanelShadowOverlayView&) = delete;

  // Call when the rounded-corners preference or split-view state changes.
  void UpdateShadowVisibility();

 private:
  // views::View:
  void AddedToWidget() override;

  // views::ViewObserver:
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewVisibilityChanged(views::View* observed_view,
                               views::View* starting_view,
                               bool visible) override;
  void OnViewIsDeleting(views::View* observed_view) override;

  void OnAnimationProgressed(const BrowserAnimationController* controller,
                             BrowserAnimationUpdate status);

  // Syncs this view's bounds, visibility, and shadow opacity to the current
  // side panel state and animation value.
  void SyncToSidePanel();

  raw_ref<BrowserView> browser_view_;
  raw_ptr<views::View> observed_panel_ = nullptr;
  raw_ptr<ShadowFrameView> shadow_box_ = nullptr;
  base::CallbackListSubscription animation_subscription_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_SIDE_PANEL_SHADOW_OVERLAY_VIEW_H_
