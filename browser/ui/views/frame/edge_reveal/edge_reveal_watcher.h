/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_REVEAL_WATCHER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_REVEAL_WATCHER_H_

#include "base/containers/enum_set.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/scoped_observation.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class View;
}  // namespace views

// Watches a host widget (typically a browser window) and reports, via a
// callback, the set of reasons that an auto-hiding edge may need to be
// revealed. Intended for use by browser "chrome" that slides into view when
// the edge is activated (e.g. the top controls in Focus Mode).
class EdgeRevealWatcher : public views::FocusChangeListener,
                          public views::WidgetObserver {
 public:
  enum class RevealReason {
    // Focus is on a descendant of a target view.
    kFocus,
    kMinValue = kFocus,

    // A visible widget is bubble-anchored to a descendant of a target view.
    kAnchoredBubble,

    // A child widget of the host widget has become currently visible. This
    // covers menus, the autocomplete popup, and any other child widgets that
    // are not bubbles anchored to a host view.
    kVisibleChildWidget,
    kMaxValue = kVisibleChildWidget,
  };

  using RevealReasons = base::
      EnumSet<RevealReason, RevealReason::kMinValue, RevealReason::kMaxValue>;

  using RevealReasonsChangedCallback =
      base::RepeatingCallback<void(RevealReasons)>;

  using EdgeContainsViewCallback =
      base::RepeatingCallback<bool(const views::View*)>;

  // Constructs a new reveal watcher for a host widget. Callers provide the
  // following callbacks:
  //   - `edge_contains_view`: A predicate that returns `true` if the specified
  //     view is contained within the edge.
  //   - `reveal_reasons_changed`: A notification that is called when the set of
  //     reveal reasons for the edge has changed.
  EdgeRevealWatcher(views::Widget* host,
                    EdgeContainsViewCallback edge_contains_view,
                    RevealReasonsChangedCallback reveal_reasons_changed);

  EdgeRevealWatcher(const EdgeRevealWatcher&) = delete;
  EdgeRevealWatcher& operator=(const EdgeRevealWatcher&) = delete;
  ~EdgeRevealWatcher() override;

  // The current set of reveal reasons, if any.
  RevealReasons reasons() const { return reasons_; }

 private:
  // views::FocusChangeListener:
  void OnDidChangeFocus(views::View* before, views::View* now) override;

  // views::WidgetObserver:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;
  void OnWidgetDestroying(views::Widget* widget) override;
  void OnWidgetChildAdded(views::Widget* widget, views::Widget* child) override;
  void OnWidgetChildRemoved(views::Widget* widget,
                            views::Widget* child) override;

  bool EdgeContainsView(const views::View* view) const;
  bool IsAnchoredToEdge(views::Widget* widget) const;

  void Recompute();

  raw_ptr<views::Widget> host_widget_;
  EdgeContainsViewCallback edge_contains_view_;
  RevealReasonsChangedCallback reveal_reasons_changed_;
  RevealReasons reasons_;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      host_observation_{this};
  base::ScopedObservation<views::FocusManager, views::FocusChangeListener>
      focus_observation_{this};
  base::ScopedMultiSourceObservation<views::Widget, views::WidgetObserver>
      child_widget_observations_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_EDGE_REVEAL_WATCHER_H_
