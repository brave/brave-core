/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_

#include <memory>
#include <optional>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "components/prefs/pref_member.h"
#include "ui/events/event_observer.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/animation/animation_delegate_views.h"
#include "ui/views/view.h"

namespace views {
class EventMonitor;
}  // namespace views

namespace sidebar {
class SidebarBrowserTest;
}  // namespace sidebar

class BraveBrowser;
class Browser;

// This view is the parent view of all sidebar ui.
// Direct child views are control view and panel view.
// Control view includes sidebar items button, add button, settings button
// and panel view includes each panel's webview.
// This class controls the visibility of control and panel view based on panel
// item state and show options.
// In the comments, we use the term "managed panel entry" and it means that
// entry is managed by sidebar model. Only managed entry has its item in sidebar
// UI.
class SidebarContainerView : public sidebar::Sidebar,
                             public SidebarControlView::Delegate,
                             public views::View,
                             public views::AnimationDelegateViews {
  METADATA_HEADER(SidebarContainerView, views::View)
 public:
  explicit SidebarContainerView(Browser* browser);
  ~SidebarContainerView() override;

  SidebarContainerView(const SidebarContainerView&) = delete;
  SidebarContainerView& operator=(const SidebarContainerView&) = delete;

  void Init();

  bool sidebar_on_left() const { return sidebar_on_left_; }
  void SetSidebarOnLeft(bool sidebar_on_left);

  bool IsSidebarVisible() const;

  // Show sidebar if the hot corner contains |point_in_screen|.
  void ShowSidebarOnMouseOver(const gfx::PointF& point_in_screen);
  void UpdateBorder();

  // Runs `callback` whenever the sidebar control view's visibility changes, so
  // the side panel can recompute its content corner radii (which depend on
  // sidebar control view visibility — see GetPanelContentsRoundedCorners()).
  void SetSidebarControlViewVisibilityChangedCallback(
      base::RepeatingClosure callback);

  // Sidebar overrides:
  void SetSidebarShowOption(
      sidebar::SidebarService::ShowSidebarOption show_option) override;
  void UpdateSidebarItemsState() override;
  void UpdateSidebarVisibility() override;

  // SidebarControlView::Delegate overrides:
  void MenuClosed() override;

  // views::View overrides:
  void ChildVisibilityChanged(views::View* child) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // views::AnimationDelegateViews overrides:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

 private:
  friend class sidebar::SidebarBrowserTest;

  class BrowserWindowEventObserver;

  // Whether the show transition should slide in or snap immediately.
  enum class AnimationStyle { kAnimated, kImmediate };

  void AddChildViews();
  bool ShouldUseAnimation();

  void ShowSidebar(AnimationStyle animation = AnimationStyle::kAnimated);
  void HideSidebar();

  bool ShouldForceShowSidebar() const;

  // true when fullscreen is initiated by tab. (Ex, fullscreen mode in youtube)
  bool IsFullscreenByTab() const;

  // On some condition(ex, add item bubble is visible),
  // sidebar should not be hidden even if mouse goes out from sidebar ui.
  // If it's hidden, only bubble ui is visible. Then, weird situation happens.
  // (Sidear UI is hidden and bubble is only visible)
  // With this handling, sidebar ui is still visible after this bubble ui is
  // disappeared. To make sidebar ui hidden, this widget's
  // event is monitored. |BrowserWindowEventObserver| will get widget's
  // mouse event when mouse is exited from sidebar ui but sidebar ui is shown.
  // |BrowserWindowEventObserver| will ask to stop monitoring and ask to hide
  // sidebar ui when those conditions are cleared and current mouse is outside
  // of sidebar ui.
  void StartBrowserWindowEventMonitoring();
  void StopBrowserWindowEventMonitoring();

  // Casts |browser_| to BraveBrowser, as storing it as BraveBrowser would cause
  // a precocious downcast.
  BraveBrowser* GetBraveBrowser() const;

  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<SidebarControlView> sidebar_control_view_ = nullptr;
  bool initialized_ = false;
  bool sidebar_on_left_ = true;
  base::OneShotTimer sidebar_hide_timer_;
  sidebar::SidebarService::ShowSidebarOption show_sidebar_option_ =
      sidebar::SidebarService::ShowSidebarOption::kShowAlways;
  gfx::SlideAnimation width_animation_{this};
  int animation_start_width_ = 0;
  int animation_end_width_ = 0;
  std::unique_ptr<BrowserWindowEventObserver> browser_window_event_observer_;
  std::unique_ptr<views::EventMonitor> browser_window_event_monitor_;
  base::RepeatingClosure sidebar_control_view_visibility_changed_callback_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_
