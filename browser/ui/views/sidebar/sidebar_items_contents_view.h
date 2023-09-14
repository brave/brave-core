/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_CONTENTS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_CONTENTS_VIEW_H_

#include <memory>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class MenuRunner;
}  // namespace views

namespace sidebar {
class SidebarBrowserTest;
}  // namespace sidebar

class BraveBrowser;
class SidebarItemView;

class SidebarItemsContentsView : public views::View,
                                 public views::ContextMenuController,
                                 public views::WidgetObserver,
                                 public ui::SimpleMenuModel::Delegate {
 public:
  METADATA_HEADER(SidebarItemsContentsView);
  SidebarItemsContentsView(BraveBrowser* browser,
                           views::DragController* drag_controller);
  ~SidebarItemsContentsView() override;

  SidebarItemsContentsView(const SidebarItemsContentsView&) = delete;
  SidebarItemsContentsView operator=(const SidebarItemsContentsView&) = delete;

  // views::View overrides:
  gfx::Size CalculatePreferredSize() const override;

  // views::ContextMenuController overrides:
  void ShowContextMenuForViewImpl(views::View* source,
                                  const gfx::Point& point,
                                  ui::MenuSourceType source_type) override;

  // views::WidgetObserver overrides:
  void OnWidgetDestroying(views::Widget* widget) override;

  // ui::SimpleMenuModel::Delegate overrides:
  void ExecuteCommand(int command_id, int event_flags) override;
  bool IsCommandIdVisible(int command_id) const override;

  void OnItemAdded(const sidebar::SidebarItem& item,
                   int index,
                   bool user_gesture);
  void OnItemMoved(const sidebar::SidebarItem& item, int from, int to);
  void OnItemRemoved(int index);
  void OnActiveIndexChanged(absl::optional<size_t> old_index,
                            absl::optional<size_t> new_index);

  void ShowItemAddedFeedbackBubble(size_t added_item_index);

  void SetImageForItem(const sidebar::SidebarItem& item,
                       const gfx::ImageSkia& image);
  void UpdateItem(const sidebar::SidebarItem& item,
                  const sidebar::SidebarItemUpdate& update);

  // |source| is drag source view.
  // |position| is in local coordinate space of |source|.
  // Returns drag indicator index.
  absl::optional<size_t> DrawDragIndicator(views::View* source,
                                           const gfx::Point& position);
  void ClearDragIndicator();

  bool IsBubbleVisible() const;
  void Update();
  void SetDefaultImageAt(int index, const sidebar::SidebarItem& item);
  void SetSidebarOnLeft(bool sidebar_on_left);

 private:
  friend class sidebar::SidebarBrowserTest;

  enum ContextMenuIDs {
    kItemEdit,
    kItemRemove,
  };

  void AddItemView(const sidebar::SidebarItem& item,
                   int index,
                   bool user_gesture);
  void UpdateItemViewStateAt(size_t index, bool active);
  bool IsBuiltInTypeItemView(views::View* view) const;

  // Called when each item is pressed.
  void OnItemPressed(const views::View* item, const ui::Event& event);
  void OnContextMenuClosed();

  ui::ImageModel GetImageForBuiltInItems(
      sidebar::SidebarItem::BuiltInItemType type,
      views::Button::ButtonState state) const;
  void UpdateAllBuiltInItemsViewState();
  void ShowItemAddedFeedbackBubble(views::View* anchor_view);

  // When item count is five, drag indicator is drawn in front of first item.
  // If |index| is 5, it's drawn after the last item.
  // Pass -1 to remove indicator.
  void DoDrawDragIndicator(absl::optional<size_t> index);
  absl::optional<size_t> CalculateTargetDragIndicatorIndex(
      const gfx::Point& screen_position);
  SidebarItemView* GetItemViewAt(size_t index);
  void LaunchEditItemDialog();

  bool sidebar_on_left_ = true;
  raw_ptr<BraveBrowser> browser_ = nullptr;
  raw_ptr<views::DragController> drag_controller_ = nullptr;
  raw_ptr<views::View> view_for_context_menu_ = nullptr;
  raw_ptr<sidebar::SidebarModel> sidebar_model_ = nullptr;
  std::unique_ptr<ui::SimpleMenuModel> context_menu_model_;
  std::unique_ptr<views::MenuRunner> context_menu_runner_;
  base::RepeatingCallback<void(views::View*)>
      item_added_bubble_launched_for_test_;
  // Observe to know whether item added feedback bubble is visible or not.
  base::ScopedObservation<views::Widget, views::WidgetObserver> observation_{
      this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_CONTENTS_VIEW_H_
