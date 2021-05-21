/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTROL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTROL_VIEW_H_

#include <memory>
#include <string>

#include "base/scoped_observer.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/view.h"

class BraveBrowser;
class SidebarItemAddButton;
class SidebarItemsScrollView;
class SidebarContainerView;

namespace views {
class MenuRunner;
}  // namespace views

// This view includes all sidebar buttons such as sidebar item buttons, add and
// settings button.
class SidebarControlView : public views::View,
                           public views::ContextMenuController,
                           public ui::SimpleMenuModel::Delegate,
                           public SidebarButtonView::Delegate,
                           public sidebar::SidebarModel::Observer {
 public:
  explicit SidebarControlView(BraveBrowser* browser);
  ~SidebarControlView() override;

  SidebarControlView(const SidebarControlView&) = delete;
  SidebarControlView& operator=(const SidebarControlView&) = delete;

  // views::View overrides:
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;

  // views::ContextMenuController overrides:
  void ShowContextMenuForViewImpl(views::View* source,
                                  const gfx::Point& point,
                                  ui::MenuSourceType source_type) override;

  // ui::SimpleMenuModel::Delegate overrides:
  void ExecuteCommand(int command_id, int event_flags) override;
  bool IsCommandIdChecked(int command_id) const override;

  // SidebarButtonView::Delegate overrides:
  std::u16string GetTooltipTextFor(const views::View* view) const override;

  // sidebar::SidebarModel::Observer overrides:
  void OnItemAdded(const sidebar::SidebarItem& item,
                   int index,
                   bool user_gesture) override;
  void OnItemRemoved(int index) override;

  void Update();

  bool IsItemReorderingInProgress() const;
  bool IsBubbleWidgetVisible() const;

 private:
  void AddChildViews();

  void OnButtonPressed(views::View* view);
  // Add button is disabled when all builtin items are enabled and current tab
  // is NTP.
  void UpdateItemAddButtonState();
  void UpdateSettingsButtonState();
  void UpdateBackgroundAndBorder();

  BraveBrowser* browser_ = nullptr;
  SidebarItemsScrollView* sidebar_items_view_ = nullptr;
  SidebarItemAddButton* sidebar_item_add_view_ = nullptr;
  SidebarButtonView* sidebar_settings_view_ = nullptr;
  std::unique_ptr<ui::SimpleMenuModel> context_menu_model_;
  std::unique_ptr<views::MenuRunner> context_menu_runner_;
  ScopedObserver<sidebar::SidebarModel, sidebar::SidebarModel::Observer>
      sidebar_model_observed_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTROL_VIEW_H_
