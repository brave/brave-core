/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_TRANSIENT_WIDGET_TRACKER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_TRANSIENT_WIDGET_TRACKER_H_

#include <memory>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "ui/views/widget/widget_observer.h"

// Tracks transient/child widgets attached to a single host widget and reports
// add/remove events to observers.
//
// "Transient/child" here is the platform-native parent/child window
// relationship, not the views view-tree parent relationship:
//   - aura: transient children of the host's aura::Window.
//   - macOS: child NSWindows of the host's NSWindow.
//
// Widgets reported here include anchored bubbles, menus, the omnibox
// autocomplete popup, drag helpers, etc.
class TransientWidgetTracker : public views::WidgetObserver {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnTransientWidgetAdded(views::Widget* widget) = 0;
    virtual void OnTransientWidgetRemoved(views::Widget* widget) = 0;
  };

  // Constructs a platform-appropriate concrete tracker for `host`.
  static std::unique_ptr<TransientWidgetTracker> Create(views::Widget* host);

  TransientWidgetTracker(const TransientWidgetTracker&) = delete;
  TransientWidgetTracker& operator=(const TransientWidgetTracker&) = delete;
  ~TransientWidgetTracker() override;

  // Adds/removes observers.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // The set of currently-tracked transient widgets.
  const base::flat_set<raw_ptr<views::Widget>>& widgets() const {
    return widgets_;
  }

 protected:
  explicit TransientWidgetTracker(views::Widget* host);

  void NotifyAdded(views::Widget* widget);
  void NotifyRemoved(views::Widget* widget);

  // Invoked when the host widget is being destroyed, before the base class
  // clears its state. Subclasses should release any platform-specific
  // observation that depends on the host's native window.
  virtual void OnHostDestroying() {}

  views::Widget* host() { return host_; }

 private:
  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  raw_ptr<views::Widget> host_;
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      host_observation_{this};
  base::flat_set<raw_ptr<views::Widget>> widgets_;
  base::ObserverList<Observer> observers_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_EDGE_REVEAL_TRANSIENT_WIDGET_TRACKER_H_
