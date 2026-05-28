/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TOP_OVERLAY_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TOP_OVERLAY_H_

#include <memory>
#include <optional>

#include "base/callback_list.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "base/types/pass_key.h"
#include "brave/browser/ui/views/frame/edge_reveal/edge_reveal_watcher.h"
#include "brave/browser/ui/views/view_shadow.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

class BraveBrowserView;
class BrowserView;
class EdgeHoverDetector;

// An auto-reveal overlay view anchored to the top edge of the browser frame.
// While active, the overlay hosts the browser's top container (and, when in
// horizontal tab mode, the horizontal tab strip nested inside it) as direct
// children, and animates its own y-offset based on hover, focus, and
// anchored bubbles. When deactivated, views are returned to their original
// placements.
//
// Layout
// ======
//
// The overlay deliberately has no layout manager of its own. Once the top
// container has been reparented as a child of this view, upstream's
// `BrowserViewLayoutImpl::Layout` detects this and runs its "top container
// parented elsewhere" branch, which sizes the top container (and lays out its
// children) inside its new parent. This is the same code path used by macOS
// immersive fullscreen, where the top container is reparented onto a separate
// overlay widget. The overlay's own bounds are derived from the top container's
// bounds via `UpdateBounds()`, kept in sync through `views::ViewObserver`.
class FocusModeTopOverlay : public views::View,
                            public views::ViewObserver,
                            public gfx::AnimationDelegate {
  METADATA_HEADER(FocusModeTopOverlay, views::View)

 public:
  using RevealFractionChangedCallback = base::RepeatingCallback<void(double)>;

  // Constructs the overlay. Instances are intended to be created and owned only
  // by a BrowserView instance.
  FocusModeTopOverlay(base::PassKey<BraveBrowserView>,
                      BrowserView* browser_view);

  FocusModeTopOverlay(const FocusModeTopOverlay&) = delete;
  FocusModeTopOverlay& operator=(const FocusModeTopOverlay&) = delete;
  ~FocusModeTopOverlay() override;

  // Begins hosting the top container and hides the overlay with an animation.
  void Activate();

  // Restores the top container views to their previous placements and hides the
  // overlay.
  void Deactivate();

  // Returns a value indicating whether the overlay is currently hosting the
  // top container views.
  bool active() const { return active_; }

  // Forces a full reveal for `duration`, then resumes state-driven behavior.
  void RevealTemporarily(base::TimeDelta duration);

  // Returns a value in [0, 1], where 1 indicates fully revealed.
  double GetRevealFraction() const;

  // Registers `callback` to be invoked whenever the reveal fraction changes.
  // The callback remains registered for the lifetime of the returned
  // subscription.
  base::CallbackListSubscription RegisterRevealFractionChanged(
      RevealFractionChangedCallback callback);

 private:
  void StopRevealing();
  bool ShouldReveal() const;
  bool ShouldRevealForChildWidget() const;
  bool IsHoverPoint(const gfx::Point& screen_point) const;
  void OnHoverStateChanged(bool hovering);
  bool EdgeContainsView(const views::View* view) const;
  void OnRevealReasonsChanged(EdgeRevealWatcher::RevealReasons reasons);
  void UpdateRevealState();
  void UpdateBounds();
  void NotifyRevealFractionChanged();

  // views::ViewObserver:
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewIsDeleting(View* observed_view) override;

  // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

  const raw_ref<BrowserView> browser_view_;
  ViewShadow shadow_;

  std::optional<size_t> top_container_index_;
  bool active_ = false;

  gfx::SlideAnimation animation_{this};
  base::OneShotTimer temporary_reveal_timer_;
  std::unique_ptr<EdgeHoverDetector> hover_detector_;
  std::unique_ptr<EdgeRevealWatcher> reveal_watcher_;

  base::ScopedObservation<views::View, views::ViewObserver>
      top_container_observation_{this};

  base::RepeatingCallbackList<void(double)> reveal_fraction_changed_callbacks_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_FOCUS_MODE_TOP_OVERLAY_H_
