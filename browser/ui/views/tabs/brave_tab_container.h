/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_

#include <memory>

#include "chrome/browser/ui/views/tabs/tab_container_impl.h"

#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/tab_drag_context.h"

class BraveTabContainer : public TabContainerImpl {
 public:
  METADATA_HEADER(BraveTabContainer);

  BraveTabContainer(TabContainerController& controller,
                    TabHoverCardController* hover_card_controller,
                    TabDragContextBase* drag_context,
                    TabSlotController& tab_slot_controller,
                    views::View* scroll_contents_view);
  ~BraveTabContainer() override;

  // Calling this will freeze this view's layout. When the returned closure
  // runs, layout will be unlocked and run immediately.
  // This is to avoid accessing invalid index during reconstruction
  // of TabContainer. In addition, we can avoid redundant layout as a side
  // effect.
  base::OnceClosure LockLayout();

  // TabContainerImpl:
  gfx::Size GetMinimumSize() const override;
  gfx::Size CalculatePreferredSize() const override;
  void UpdateClosingModeOnRemovedTab(int model_index, bool was_active) override;
  gfx::Rect GetTargetBoundsForClosingTab(Tab* tab,
                                         int former_model_index) const override;
  void EnterTabClosingMode(absl::optional<int> override_width,
                           CloseTabSource source) override;
  bool ShouldTabBeVisible(const Tab* tab) const override;
  void StartInsertTabAnimation(int model_index) override;
  void RemoveTab(int index, bool was_active) override;
  void OnTabCloseAnimationCompleted(Tab* tab) override;
  void CompleteAnimationAndLayout() override;
  void PaintChildren(const views::PaintInfo& paint_info) override;

  // BrowserRootView::DropTarget
  BrowserRootView::DropIndex GetDropIndex(
      const ui::DropTargetEvent& event) override;
  void HandleDragUpdate(
      const absl::optional<BrowserRootView::DropIndex>& index) override;
  void HandleDragExited() override;

 private:
  class DropArrow : public views::WidgetObserver {
   public:
    enum class Position { Vertical, Horizontal };

    DropArrow(const BrowserRootView::DropIndex& index,
              Position position,
              bool beneath,
              views::Widget* context);
    DropArrow(const DropArrow&) = delete;
    DropArrow& operator=(const DropArrow&) = delete;
    ~DropArrow() override;

    void set_index(const BrowserRootView::DropIndex& index) { index_ = index; }
    BrowserRootView::DropIndex index() const { return index_; }

    void SetBeneath(bool beneath);
    bool beneath() const { return beneath_; }

    void SetWindowBounds(const gfx::Rect& bounds);

    // views::WidgetObserver:
    void OnWidgetDestroying(views::Widget* widget) override;

   private:
    // Index of the tab to drop on.
    BrowserRootView::DropIndex index_;

    Position position_ = Position::Vertical;

    bool beneath_ = false;

    // Renders the drop indicator.
    raw_ptr<views::Widget, DanglingUntriaged> arrow_window_ = nullptr;

    raw_ptr<views::ImageView, DanglingUntriaged> arrow_view_ = nullptr;

    base::ScopedObservation<views::Widget, views::WidgetObserver>
        scoped_observation_{this};
  };

  void UpdateLayoutOrientation();

  static gfx::ImageSkia* GetDropArrowImage(
      BraveTabContainer::DropArrow::Position pos,
      bool beneath);

  void OnUnlockLayout();

  void SetDropArrow(const absl::optional<BrowserRootView::DropIndex>& index);
  gfx::Rect GetDropBounds(int drop_index,
                          bool drop_before,
                          bool drop_in_group,
                          bool* is_beneath);

  base::flat_set<Tab*> closing_tabs_;

  raw_ptr<TabDragContext> drag_context_;

  // A pointer storing the global tab style to be used.
  const raw_ptr<const TabStyle> tab_style_;

  const raw_ref<TabContainerController, DanglingUntriaged> controller_;

  std::unique_ptr<DropArrow> drop_arrow_;

  BooleanPrefMember show_vertical_tabs_;
  BooleanPrefMember vertical_tabs_floating_mode_enabled_;
  BooleanPrefMember vertical_tabs_collapsed_;

  bool layout_locked_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_
