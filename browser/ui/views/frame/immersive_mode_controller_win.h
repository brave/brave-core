/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_MODE_CONTROLLER_WIN_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_MODE_CONTROLLER_WIN_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/views/frame/immersive_fullscreen_controller_delegate.h"
#include "brave/browser/ui/views/frame/immersive_fullscreen_controller_win.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/immersive_mode_controller.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view_observer.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

class ImmersiveModeControllerWin;

// This class notifies the browser view to refresh layout whenever the overlay
// widget moves. This is necessary for positioning web dialogs.
class ImmersiveModeOverlayWidgetObserver : public views::WidgetObserver {
 public:
  explicit ImmersiveModeOverlayWidgetObserver(
      ImmersiveModeControllerWin* controller);

  ImmersiveModeOverlayWidgetObserver(
      const ImmersiveModeOverlayWidgetObserver&) = delete;
  ImmersiveModeOverlayWidgetObserver& operator=(
      const ImmersiveModeOverlayWidgetObserver&) = delete;
  ~ImmersiveModeOverlayWidgetObserver() override;

  // views::WidgetObserver:
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

 private:
  raw_ptr<ImmersiveModeControllerWin> controller_;
};

class ImmersiveModeControllerWin : public ImmersiveModeController,
                                   public views::FocusChangeListener,
                                   public ImmersiveFullscreenControllerDelegate,
                                   public views::ViewObserver,
                                   public views::WidgetObserver,
                                   public views::FocusTraversable {
 public:
  ImmersiveModeControllerWin();

  ImmersiveModeControllerWin(const ImmersiveModeControllerWin&) = delete;
  ImmersiveModeControllerWin& operator=(const ImmersiveModeControllerWin&) =
      delete;

  ~ImmersiveModeControllerWin() override;

  ImmersiveFullscreenControllerWin* controller() { return &controller_; }

  // ImmersiveModeController overrides:
  void Init(BrowserView* browser_view) override;
  void SetEnabled(bool enabled) override;
  bool IsEnabled() const override;
  bool ShouldHideTopViews() const override;
  bool IsRevealed() const override;
  int GetTopContainerVerticalOffset(
      const gfx::Size& top_container_size) const override;
  std::unique_ptr<ImmersiveRevealedLock> GetRevealedLock(
      AnimateReveal animate_reveal) override;
  void OnFindBarVisibleBoundsChanged(
      const gfx::Rect& new_visible_bounds_in_screen) override;
  bool ShouldStayImmersiveAfterExitingFullscreen() override;
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override;
  int GetMinimumContentOffset() const override;
  int GetExtraInfobarOffset() const override;
  void OnContentFullscreenChanged(bool is_content_fullscreen) override {}

  // views::FocusChangeListener implementation.
  void OnWillChangeFocus(views::View* focused_before,
                         views::View* focused_now) override;
  void OnDidChangeFocus(views::View* focused_before,
                        views::View* focused_now) override;

  // views::ViewObserver implementation
  void OnViewBoundsChanged(views::View* observed_view) override;

  // views::WidgetObserver implementation
  void OnWidgetDestroying(views::Widget* widget) override;

  // views::Traversable:
  views::FocusSearch* GetFocusSearch() override;
  views::FocusTraversable* GetFocusTraversableParent() override;
  views::View* GetFocusTraversableParentView() override;

  BrowserView* browser_view() { return browser_view_; }

 private:
  // Move children from `from_widget` to `to_widget`. Certain child widgets will
  // be held back from the move, see `ShouldMoveChild` for details.
  void MoveChildren(views::Widget* from_widget, views::Widget* to_widget);

  // Returns true if the child should be moved.
  bool ShouldMoveChild(views::Widget* child);

  // ImmersiveFullscreenController::Delegate overrides:
  void OnImmersiveRevealStarted() override;
  void OnImmersiveRevealEnded() override;
  void OnImmersiveFullscreenEntered() override;
  void OnImmersiveFullscreenExited() override;
  void SetVisibleFraction(double visible_fraction) override;
  std::vector<gfx::Rect> GetVisibleBoundsInScreen() const override;

  ImmersiveFullscreenControllerWin controller_;

  raw_ptr<BrowserView> browser_view_ = nullptr;  // weak
  std::unique_ptr<ImmersiveRevealedLock> focus_lock_;
  base::ScopedObservation<views::View, views::ViewObserver>
      top_container_observation_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      browser_frame_observation_{this};
  ImmersiveModeOverlayWidgetObserver overlay_widget_observer_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      overlay_widget_observation_{&overlay_widget_observer_};
  std::unique_ptr<views::FocusSearch> focus_search_;

  // The current visible bounds of the find bar, in screen coordinates. This is
  // an empty rect if the find bar is not visible.
  gfx::Rect find_bar_visible_bounds_in_screen_;

  // The fraction of the TopContainerView's height which is visible. Zero when
  // the top-of-window views are not revealed.
  double visible_fraction_ = 1.0;

  base::WeakPtrFactory<ImmersiveModeControllerWin> weak_ptr_factory_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_IMMERSIVE_MODE_CONTROLLER_WIN_H_
