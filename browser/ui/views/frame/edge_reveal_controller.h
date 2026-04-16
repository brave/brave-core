/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_CONTROLLER_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "ui/events/event_observer.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class EventMonitor;
class View;
class Widget;
}  // namespace views

// Manages slide-hide/reveal of a set of views along a frame edge.
//
// Views are hidden by reporting a fractional offset that the layout system
// applies to their positions. They are revealed when:
//   - The mouse enters a hot zone along the edge.
//   - A registered view (or descendant) has focus.
//   - A visible bubble is anchored to a registered view.
//
// Not tied to any specific feature. Intended to be owned per-window.
class EdgeRevealController : public ui::EventObserver,
                             public views::FocusChangeListener,
                             public views::WidgetObserver,
                             public gfx::AnimationDelegate {
 public:
  enum class Edge { kTop, kLeft, kBottom, kRight };

  struct RevealableOptions {
    bool paint_to_layer = false;
  };

  class Observer : public base::CheckedObserver {
   public:
    // |fraction| ranges from 0 (fully hidden) to 1 (fully shown).
    virtual void OnEdgeRevealFractionChanged(double fraction) = 0;
  };

  EdgeRevealController(Edge edge, views::Widget* widget);
  EdgeRevealController(const EdgeRevealController&) = delete;
  EdgeRevealController& operator=(const EdgeRevealController&) = delete;
  ~EdgeRevealController() override;

  void AddRevealableView(views::View* view,
                         RevealableOptions options = {.paint_to_layer = false});
  void RemoveRevealableView(views::View* view);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void SetEnabled(bool enabled);
  bool IsEnabled() const { return enabled_; }

  // 0 = fully hidden, 1 = fully shown.
  double GetRevealFraction() const;

  bool IsRevealed() const;

  // Forces a reveal for |duration|, then resumes normal state-driven reveal.
  // No-op when disabled. Calling again restarts the timer with the new
  // duration.
  void RevealTemporarily(base::TimeDelta duration);

 private:
  class RevealableState;
  using RevealableViewMap =
      base::flat_map<raw_ptr<views::View>, std::unique_ptr<RevealableState>>;

  bool ShouldReveal() const;
  bool HasFocusInRevealableViews() const;
  bool ContainsView(const views::View* view) const;

  void UpdateRevealState();
  void ScanForAnchoredBubbles();

  void SetHoveringWithDelay(bool hovering);
  void CommitHovering(bool hovering);

  // ui::EventObserver:
  void OnEvent(const ui::Event& event) override;

  // views::FocusChangeListener:
  void OnDidChangeFocus(views::View* before, views::View* after) override;

  // views::WidgetObserver:
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

  const Edge edge_;
  bool enabled_ = false;
  bool hovering_ = false;
  bool temporary_reveal_ = false;

  raw_ptr<views::Widget> widget_;
  base::ObserverList<Observer> observers_;
  gfx::SlideAnimation animation_{this};
  std::unique_ptr<views::EventMonitor> event_monitor_;
  RevealableViewMap revealable_views_;
  base::flat_set<raw_ptr<views::Widget>> observed_bubble_widgets_;

  base::OneShotTimer mouse_hover_timer_;
  base::OneShotTimer temporary_reveal_timer_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_CONTROLLER_H_
