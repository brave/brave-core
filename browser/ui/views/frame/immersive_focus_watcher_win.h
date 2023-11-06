/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_FOCUS_WATCHER_WIN_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_FOCUS_WATCHER_WIN_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/aura/client/transient_window_client_observer.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/wm/public/activation_change_observer.h"

class ImmersiveFullscreenControllerWin;
class SimpleImmersiveRevealedLock;

// ImmersiveFocusWatcherWin is responsible for grabbing a reveal lock based on
// activation and/or focus. This implementation grabs a lock if views focus is
// in the top view, a bubble is showing that is anchored to the top view, or the
// focused window is a transient child of the top view's widget.
class ImmersiveFocusWatcherWin
    : public views::FocusChangeListener,
      public aura::client::TransientWindowClientObserver,
      public ::wm::ActivationChangeObserver {
 public:
  explicit ImmersiveFocusWatcherWin(
      ImmersiveFullscreenControllerWin* controller);

  ImmersiveFocusWatcherWin(const ImmersiveFocusWatcherWin&) = delete;
  ImmersiveFocusWatcherWin& operator=(const ImmersiveFocusWatcherWin&) = delete;

  ~ImmersiveFocusWatcherWin() override;

  // Forces updating the status of the lock. That is, this determines whether
  // a lock should be held and updates accordingly. The lock is automatically
  // maintained, but this function may be called to force an update.
  void UpdateFocusRevealedLock();

  // Explicitly releases the lock, does nothing if a lock is not held.
  void ReleaseLock();

 private:
  class BubbleObserver;

  views::Widget* GetWidget();
  aura::Window* GetWidgetWindow();

  // Recreate |bubble_observer_| and start observing any bubbles anchored to a
  // child of |top_container_|.
  void RecreateBubbleObserver();

  // views::FocusChangeListener overrides:
  void OnWillChangeFocus(views::View* focused_before,
                         views::View* focused_now) override;
  void OnDidChangeFocus(views::View* focused_before,
                        views::View* focused_now) override;

  // aura::client::TransientWindowClientObserver overrides:
  void OnTransientChildWindowAdded(aura::Window* window,
                                   aura::Window* transient) override;
  void OnTransientChildWindowRemoved(aura::Window* window,
                                     aura::Window* transient) override;

  // ::wm::ActivationChangeObserver:
  void OnWindowActivated(
      ::wm::ActivationChangeObserver::ActivationReason reason,
      aura::Window* gaining_active,
      aura::Window* losing_active) override;

  raw_ptr<ImmersiveFullscreenControllerWin> immersive_fullscreen_controller_;

  // Lock which keeps the top-of-window views revealed based on the focused view
  // and the active widget. Acquiring the lock never triggers a reveal because
  // a view is not focusable till a reveal has made it visible.
  std::unique_ptr<SimpleImmersiveRevealedLock> lock_;

  // Manages bubbles which are anchored to a child of
  // |ImmersiveFullscreenController::top_container_|.
  std::unique_ptr<BubbleObserver> bubble_observer_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_FOCUS_WATCHER_WIN_H_
