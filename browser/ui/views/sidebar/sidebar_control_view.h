/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTROL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTROL_VIEW_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "ui/menus/simple_menu_model.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/view.h"

class BraveBrowser;
class SidebarItemAddButton;
class SidebarItemsScrollView;
class SidebarContainerView;

namespace views {
class MenuRunner;
}  // namespace views

namespace sidebar {
class SidebarBrowserTest;
}  // namespace sidebar

// This view includes all sidebar buttons such as sidebar item buttons, add and
// settings button.
class SidebarControlView : public views::View,
                           public views::ContextMenuController,
                           public ui::SimpleMenuModel::Delegate,
                           public sidebar::SidebarModel::Observer {
  METADATA_HEADER(SidebarControlView, views::View)
 public:
  class Delegate {
   public:
    virtual void MenuClosed() {}

   protected:
    ~Delegate() = default;
  };
  SidebarControlView(Delegate* delegate, BraveBrowser* browser);
  ~SidebarControlView() override;

  SidebarControlView(const SidebarControlView&) = delete;
  SidebarControlView& operator=(const SidebarControlView&) = delete;

  // views::View overrides:
  void OnThemeChanged() override;

  // views::ContextMenuController overrides:
  void ShowContextMenuForViewImpl(
      views::View* source,
      const gfx::Point& point,
      ui::mojom::MenuSourceType source_type) override;

  // ui::SimpleMenuModel::Delegate overrides:
  void ExecuteCommand(int command_id, int event_flags) override;
  bool IsCommandIdChecked(int command_id) const override;
  void MenuClosed(ui::SimpleMenuModel* source) override;

  // sidebar::SidebarModel::Observer overrides:
  void OnItemAdded(const sidebar::SidebarItem& item,
                   size_t index,
                   bool user_gesture) override;
  void OnItemRemoved(size_t index) override;

  void Update();

  bool IsItemReorderingInProgress() const;
  bool IsBubbleWidgetVisible() const;
  void SetSidebarOnLeft(bool sidebar_on_left);

 private:
  friend class sidebar::SidebarBrowserTest;

  void AddChildViews();

  void OnButtonPressed(views::View* view);
  // Add button is disabled when all builtin items are enabled and current tab
  // is NTP.
  void UpdateItemAddButtonState();
  void UpdateSettingsButtonState();
  void UpdateBackgroundAndBorder();

  bool sidebar_on_left_ = true;
  raw_ptr<Delegate> delegate_ = nullptr;
  raw_ptr<BraveBrowser> browser_ = nullptr;
  raw_ptr<SidebarItemsScrollView> sidebar_items_view_ = nullptr;
  raw_ptr<SidebarItemAddButton> sidebar_item_add_view_ = nullptr;
  raw_ptr<SidebarButtonView> sidebar_settings_view_ = nullptr;
  std::unique_ptr<ui::SimpleMenuModel> context_menu_model_;
  std::unique_ptr<views::MenuRunner> context_menu_runner_;
  base::ScopedObservation<sidebar::SidebarModel,
                          sidebar::SidebarModel::Observer>
      sidebar_model_observed_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_CONTROL_VIEW_H_
