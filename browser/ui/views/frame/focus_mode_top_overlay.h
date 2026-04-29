/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TOP_OVERLAY_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TOP_OVERLAY_H_

#include <memory>
#include <vector>

#include "base/callback_list.h"
#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/timer/timer.h"
#include "brave/browser/ui/views/view_shadow.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class Widget;
}  // namespace views

// Slide-hide/reveal overlay anchored to the top edge of its parent.
//
// While active, the overlay hosts the browser's top container (and, when in
// horizontal tab mode, the horizontal tab strip nested inside it) as direct
// children, and animates its own y-offset based on hover, focus, and
// anchored bubbles. When deactivated, hosted views are returned to their
// original parents/indices.
class FocusModeTopOverlay : public views::View,
                            public views::FocusChangeListener,
                            public views::ViewObserver,
                            public views::WidgetObserver,
                            public gfx::AnimationDelegate {
  METADATA_HEADER(FocusModeTopOverlay, views::View)

 public:
  // Views to host while the overlay is active. `top_container` is required;
  // `horizontal_tab_strip` is optional and is re-parented into
  // `top_container` so that the upstream tabbed layout positions it
  // correctly while `top_container` is itself hosted inside the overlay.
  // If `horizontal_tab_strip` is already a child of `top_container` the
  // re-parenting step is skipped.
  struct HostedViews {
    raw_ptr<views::View> top_container = nullptr;
    raw_ptr<views::View> horizontal_tab_strip = nullptr;
  };

  using RevealFractionChangedCallbackList =
      base::RepeatingCallbackList<void(double)>;
  using RevealFractionChangedCallback =
      RevealFractionChangedCallbackList::CallbackType;

  FocusModeTopOverlay();
  FocusModeTopOverlay(const FocusModeTopOverlay&) = delete;
  FocusModeTopOverlay& operator=(const FocusModeTopOverlay&) = delete;
  ~FocusModeTopOverlay() override;

  // Begins hosting the views in `views` as children, starts state-driven
  // reveal, and shows. Records each hosted view's previous parent and
  // child index so they can be restored on Deactivate(). Must be called
  // after the overlay has been parented into a widget.
  void Activate(HostedViews views);

  // Restores hosted views to their previous parents/indices, stops reveal
  // logic, and hides. Idempotent.
  void Deactivate();

  bool IsActive() const { return active_; }

  // Forces a full reveal for `duration`, then resumes state-driven
  // behavior. No-op when inactive. Calling again restarts the timer with
  // the new duration.
  void RevealTemporarily(base::TimeDelta duration);

  // Returns a value in [0, 1], where 1 indicates fully revealed.
  double GetRevealFraction() const;

  // Registers `callback` to be invoked whenever the reveal fraction
  // changes. The returned subscription is owned by the caller.
  base::CallbackListSubscription RegisterRevealFractionChangedCallback(
      RevealFractionChangedCallback callback);

 private:
  class WindowEventHandler;

  struct HostedView {
    raw_ptr<views::View> view;
    raw_ptr<views::View> previous_parent;
    size_t previous_index;
  };

  void StopRevealing();

  bool ShouldReveal() const;
  bool HasFocusInHostedViews() const;
  bool IsPointInHostedViews(gfx::Point point) const;
  bool ContainsView(const views::View* view) const;

  void UpdateRevealState();
  void UpdateBounds();
  void ScanForAnchoredBubbles();

  void SetHoveringWithDelay(bool hovering);
  void OnHoverTimerElapsed(bool hovering);
  void OnTemporaryRevealTimerElapsed();

  // Invoked by `window_event_handler_` for every mouse-move / mouse-exit
  // event in the overlay's widget.
  void OnWindowEvent(const ui::Event& event);

  // views::FocusChangeListener:
  void OnDidChangeFocus(views::View* before, views::View* after) override;

  // views::ViewObserver:
  void OnViewBoundsChanged(views::View* observed_view) override;

  // views::WidgetObserver:
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

  ViewShadow shadow_;

  bool active_ = false;
  bool hovering_ = false;
  bool temporary_reveal_ = false;

  std::vector<HostedView> hosted_views_;
  raw_ptr<views::View> hosted_top_container_ = nullptr;

  gfx::SlideAnimation animation_{this};
  std::unique_ptr<WindowEventHandler> window_event_handler_;
  base::flat_set<raw_ptr<views::Widget>> observed_bubble_widgets_;

  base::ScopedMultiSourceObservation<views::View, views::ViewObserver>
      view_observations_{this};

  base::OneShotTimer mouse_hover_timer_;
  base::OneShotTimer temporary_reveal_timer_;

  RevealFractionChangedCallbackList reveal_fraction_changed_callbacks_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TOP_OVERLAY_H_
