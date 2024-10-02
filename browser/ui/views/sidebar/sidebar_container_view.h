/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_

#include <memory>
#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_show_options_event_detect_widget.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_observer.h"
#include "chrome/browser/ui/views/side_panel/side_panel_view_state_observer.h"
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
class SidePanelEntry;

// This view is the parent view of all sidebar ui.
// Direct child views are control view and panel view.
// Control view includes sidebar items button, add button, settings button
// and panel view includes each panel's webview.
// This class controls the visibility of control and panel view based on panel
// item state and show options.
// In the comments, we use the term "managed panel entry" and it means that
// entry is managed by sidebar model. Only managed entry has its item in sidebar
// UI.
class SidebarContainerView
    : public sidebar::Sidebar,
      public SidebarControlView::Delegate,
      public views::View,
      public views::AnimationDelegateViews,
      public SidebarShowOptionsEventDetectWidget::Delegate,
      public sidebar::SidebarModel::Observer,
      public SidePanelEntryObserver,
      public SidePanelViewStateObserver,
      public TabStripModelObserver {
  METADATA_HEADER(SidebarContainerView, views::View)
 public:
  SidebarContainerView(Browser* browser,
                       SidePanelCoordinator* side_panel_coordinator,
                       std::unique_ptr<BraveSidePanel> side_panel);
  ~SidebarContainerView() override;

  SidebarContainerView(const SidebarContainerView&) = delete;
  SidebarContainerView& operator=(const SidebarContainerView&) = delete;

  void Init();

  bool sidebar_on_left() const { return sidebar_on_left_; }
  void SetSidebarOnLeft(bool sidebar_on_left);

  bool IsSidebarVisible() const;

  BraveSidePanel* side_panel() { return side_panel_; }

  void WillShowSidePanel();
  void WillDeregisterSidePanelEntry(SidePanelEntry* entry);
  bool IsFullscreenForCurrentEntry() const;

  void set_operation_from_active_tab_change(bool tab_change) {
    operation_from_active_tab_change_ = tab_change;
  }

  // Sidebar overrides:
  void SetSidebarShowOption(
      sidebar::SidebarService::ShowSidebarOption show_option) override;
  void UpdateSidebarItemsState() override;

  // SidebarControlView::Delegate overrides:
  void MenuClosed() override;

  // views::View overrides:
  void Layout(PassKey) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // views::AnimationDelegateViews overrides:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

  // SidebarShowOptionsEventDetectWidget::Delegate overrides:
  void ShowSidebarControlView() override;

  // sidebar::SidebarModel::Observer overrides:
  void OnItemAdded(const sidebar::SidebarItem& item,
                   size_t index,
                   bool user_gesture) override;
  void OnActiveIndexChanged(std::optional<size_t> old_index,
                            std::optional<size_t> new_index) override;
  void OnItemRemoved(size_t index) override;

  // NOTE: If SidePanelEntryObserver could be used from outside of view,
  // SidebarController should become a SidePanelEntryObserver.
  // SidePanelEntryObserver:
  void OnEntryShown(SidePanelEntry* entry) override;
  void OnEntryHidden(SidePanelEntry* entry) override;

  // TabStripModelObserver:
  void OnTabWillBeRemoved(content::WebContents* contents, int index) override;

  // SidePanelViewStateObserver:
  void OnSidePanelDidClose() override;

 private:
  friend class sidebar::SidebarBrowserTest;

  class BrowserWindowEventObserver;

  void AddChildViews();
  void UpdateBackground();
  void ShowOptionsEventDetectWidget(bool show);
  bool ShouldUseAnimation();

  // Show control view. panel's visibility depends on |show_side_panel|.
  void ShowSidebar(bool show_side_panel);

  // Show all (panel + control view).
  void ShowSidebarAll();

  // Hide panel. Control view's visibility depends on |hide_sidebar_control|.
  void HideSidebar(bool hide_sidebbar_control);

  // Hide all(panel + control view).
  void HideSidebarAll();
  void HideSidebarPanel();

  // Panel is hidden and control view visibility is controlled by show option.
  // Call this when want to hide and only want to consider show opton.
  void HideSidebarForShowOption();

  SidebarShowOptionsEventDetectWidget* GetEventDetectWidget();
  bool ShouldForceShowSidebar() const;
  void UpdateToolbarButtonVisibility();

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

  void StartObservingContextualSidePanelEntry(content::WebContents* contents);
  void StopObservingContextualSidePanelEntry(content::WebContents* contents);
  void StartObservingForEntry(SidePanelEntry* entry);
  void StopObservingForEntry(SidePanelEntry* entry);
  void UpdateActiveItemState();

  // Casts |browser_| to BraveBrowser, as storing it as BraveBrowser would cause
  // a precocious downcast.
  BraveBrowser* GetBraveBrowser() const;

  raw_ptr<Browser> browser_ = nullptr;
  raw_ptr<SidePanelCoordinator> side_panel_coordinator_ = nullptr;
  raw_ptr<BraveSidePanel> side_panel_ = nullptr;
  raw_ptr<sidebar::SidebarModel> sidebar_model_ = nullptr;
  raw_ptr<SidebarControlView> sidebar_control_view_ = nullptr;
  bool initialized_ = false;
  bool sidebar_on_left_ = true;
  bool operation_from_active_tab_change_ = false;
  base::OneShotTimer sidebar_hide_timer_;
  sidebar::SidebarService::ShowSidebarOption show_sidebar_option_ =
      sidebar::SidebarService::ShowSidebarOption::kShowAlways;
  gfx::SlideAnimation width_animation_{this};
  int animation_start_width_ = 0;
  int animation_end_width_ = 0;
  std::unique_ptr<BrowserWindowEventObserver> browser_window_event_observer_;
  std::unique_ptr<views::EventMonitor> browser_window_event_monitor_;
  std::unique_ptr<SidebarShowOptionsEventDetectWidget> show_options_widget_;
  BooleanPrefMember show_side_panel_button_;
  base::ScopedObservation<SidePanelCoordinator, SidePanelViewStateObserver>
      side_panel_view_state_observation_{this};
  base::ScopedObservation<sidebar::SidebarModel,
                          sidebar::SidebarModel::Observer>
      sidebar_model_observation_{this};
  base::ScopedMultiSourceObservation<SidePanelEntry, SidePanelEntryObserver>
      panel_entry_observations_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_
