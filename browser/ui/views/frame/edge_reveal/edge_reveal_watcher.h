/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_REVEAL_WATCHER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_REVEAL_WATCHER_H_

#include <memory>

#include "base/containers/enum_set.h"
#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/views/frame/edge_reveal/transient_widget_tracker.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class View;
}  // namespace views

// Watches a host widget and reports, via a callback, the set of reasons that an
// edge may need to be revealed. Intended for use by edge-mounted, auto-hiding
// browser "chrome".
class EdgeRevealWatcher : public TransientWidgetTracker::Observer,
                          public views::FocusChangeListener,
                          public views::WidgetObserver {
 public:
  enum class RevealReason {
    // Focus is on a descendant of one of the target views.
    kFocus,
    kMinValue = kFocus,

    // A visible widget is bubble-anchored to a descendant of one of the target
    // views.
    kAnchoredBubble,

    // A transient/child widget of the host widget has become currently visible.
    // This covers menus, the autocomplete popup, and any other auxiliary
    // widgets that take key but are not bubbles anchored to a target view.
    kVisibleTransientChild,
    kMaxValue = kVisibleTransientChild,
  };

  using RevealReasons = base::
      EnumSet<RevealReason, RevealReason::kMinValue, RevealReason::kMaxValue>;

  using RevealReasonsChangedCallback =
      base::RepeatingCallback<void(RevealReasons)>;

  using EdgeContainsViewCallback =
      base::RepeatingCallback<bool(const views::View*)>;

  EdgeRevealWatcher(views::Widget* host,
                    EdgeContainsViewCallback edge_contains_view,
                    RevealReasonsChangedCallback reveal_reasons_changed,
                    std::unique_ptr<TransientWidgetTracker> tracker = {});

  EdgeRevealWatcher(const EdgeRevealWatcher&) = delete;
  EdgeRevealWatcher& operator=(const EdgeRevealWatcher&) = delete;
  ~EdgeRevealWatcher() override;

  // The current set of reveal reasons, if any.
  RevealReasons reasons() const { return reasons_; }

 private:
  // TransientWidgetTracker::Observer:
  void OnTransientWidgetAdded(views::Widget* widget) override;
  void OnTransientWidgetRemoved(views::Widget* widget) override;

  // views::FocusChangeListener:
  void OnDidChangeFocus(views::View* before, views::View* now) override;

  // views::WidgetObserver:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  bool IsInAnyTarget(const views::View* view) const;
  bool IsAnchoredInTarget(views::Widget* widget) const;

  void Recompute();

  raw_ptr<views::Widget> host_widget_;
  EdgeContainsViewCallback edge_contains_view_;
  RevealReasonsChangedCallback reveal_reasons_changed_;
  std::unique_ptr<TransientWidgetTracker> tracker_;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      host_observation_{this};
  base::ScopedObservation<views::FocusManager, views::FocusChangeListener>
      focus_observation_{this};
  base::ScopedObservation<TransientWidgetTracker,
                          TransientWidgetTracker::Observer>
      tracker_observation_{this};

  base::flat_set<raw_ptr<views::Widget>> transients_;
  base::ScopedMultiSourceObservation<views::Widget, views::WidgetObserver>
      transient_observations_{this};

  RevealReasons reasons_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_REVEAL_WATCHER_H_
