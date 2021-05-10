/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_

#include <memory>

#include "base/scoped_observer.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/views/sidebar/sidebar_show_options_event_detect_widget.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "ui/views/view.h"

namespace views {
class WebView;
}  // namespace views

class BraveBrowser;

class SidebarControlView;

// This view is the parent view of all sidebar ui.
// Thi will include sidebar items button, add button, settings button and panel
// view.
class SidebarContainerView
    : public sidebar::Sidebar,
      public views::View,
      public SidebarShowOptionsEventDetectWidget::Delegate,
      public sidebar::SidebarModel::Observer {
 public:
  explicit SidebarContainerView(BraveBrowser* browser);
  ~SidebarContainerView() override;

  SidebarContainerView(const SidebarContainerView&) = delete;
  SidebarContainerView& operator=(const SidebarContainerView&) = delete;

  void Init();

  // Sidebar overrides:
  void SetSidebarShowOption(
      sidebar::SidebarService::ShowSidebarOption show_option) override;
  void UpdateSidebar() override;

  // views::View overrides:
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // SidebarShowOptionsEventDetectWidget::Delegate overrides:
  void ShowSidebar() override;

  // sidebar::SidebarModel::Observer overrides:
  void OnActiveIndexChanged(int old_index, int new_index) override;

 private:
  void AddChildViews();
  void UpdateBackgroundAndBorder();
  void UpdateChildViewVisibility();
  void ShowOptionsEventDetectWidget(bool show);
  void ShowSidebar(bool show_sidebar, bool show_event_detect_widget);
  SidebarShowOptionsEventDetectWidget* GetEventDetectWidget();

  BraveBrowser* browser_ = nullptr;
  sidebar::SidebarModel* sidebar_model_ = nullptr;
  views::WebView* sidebar_panel_view_ = nullptr;
  SidebarControlView* sidebar_control_view_ = nullptr;
  bool initialized_ = false;
  std::unique_ptr<SidebarShowOptionsEventDetectWidget> show_options_widget_;
  ScopedObserver<sidebar::SidebarModel, sidebar::SidebarModel::Observer>
      observed_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTAINER_VIEW_H_
