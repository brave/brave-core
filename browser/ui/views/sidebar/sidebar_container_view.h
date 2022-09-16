/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_show_options_event_detect_widget.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_observer.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry_observer.h"
#include "ui/events/event_observer.h"
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
// Thi will include sidebar items button, add button, settings button and panel
// view.
class SidebarContainerView
    : public sidebar::Sidebar,
      public SidebarControlView::Delegate,
      public views::View,
      public SidebarShowOptionsEventDetectWidget::Delegate,
      public sidebar::SidebarModel::Observer,
      public SidePanelEntryObserver,
      public SidePanelRegistryObserver {
 public:
  METADATA_HEADER(SidebarContainerView);
  SidebarContainerView(BraveBrowser* browser,
                       SidePanelCoordinator* side_panel_coordinator,
                       std::unique_ptr<BraveSidePanel> side_panel);
  ~SidebarContainerView() override;

  SidebarContainerView(const SidebarContainerView&) = delete;
  SidebarContainerView& operator=(const SidebarContainerView&) = delete;

  void Init();

  BraveSidePanel* side_panel() { return side_panel_; }

  // Sidebar overrides:
  void SetSidebarShowOption(
      sidebar::SidebarService::ShowSidebarOption show_option) override;
  void UpdateSidebar() override;

  // SidebarControlView::Delegate overrides:
  void MenuClosed() override;

  // views::View overrides:
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // SidebarShowOptionsEventDetectWidget::Delegate overrides:
  void ShowSidebar() override;

  // sidebar::SidebarModel::Observer overrides:
  void OnItemAdded(const sidebar::SidebarItem& item,
                   size_t index,
                   bool user_gesture) override;
  void OnActiveIndexChanged(absl::optional<size_t> old_index,
                            absl::optional<size_t> new_index) override;
  void OnItemRemoved(size_t index) override;

  // SidePanelEntryObserver:
  void OnEntryShown(SidePanelEntry* entry) override;
  void OnEntryHidden(SidePanelEntry* entry) override;

  // SidePanelRegistryObserver:
  void OnEntryRegistered(SidePanelEntry* entry) override;
  void OnEntryWillDeregister(SidePanelEntry* entry) override;

 private:
  friend class sidebar::SidebarBrowserTest;

  class BrowserWindowEventObserver;

  void AddChildViews();
  void UpdateBackground();
  void UpdateSidebarVisibility();
  void UpdateSidebarVisibility(
      sidebar::SidebarService::ShowSidebarOption show_option);
  void ShowOptionsEventDetectWidget(bool show);
  void ShowSidebar(bool show_sidebar, bool show_event_detect_widget);
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

  void DoHideSidebar(bool show_event_detect_widget);

  raw_ptr<BraveBrowser> browser_ = nullptr;
  raw_ptr<SidePanelCoordinator> side_panel_coordinator_ = nullptr;
  raw_ptr<BraveSidePanel> side_panel_ = nullptr;
  raw_ptr<sidebar::SidebarModel> sidebar_model_ = nullptr;
  raw_ptr<SidebarControlView> sidebar_control_view_ = nullptr;
  bool initialized_ = false;
  bool is_side_panel_event_ = false;
  base::OneShotTimer sidebar_hide_timer_;
  std::unique_ptr<BrowserWindowEventObserver> browser_window_event_observer_;
  std::unique_ptr<views::EventMonitor> browser_window_event_monitor_;
  std::unique_ptr<SidebarShowOptionsEventDetectWidget> show_options_widget_;
  base::ScopedObservation<sidebar::SidebarModel,
                          sidebar::SidebarModel::Observer>
      sidebar_model_observation_{this};
  base::ScopedMultiSourceObservation<SidePanelEntry, SidePanelEntryObserver>
      panel_entry_observations_{this};
  base::ScopedObservation<SidePanelRegistry, SidePanelRegistryObserver>
      panel_registry_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_
