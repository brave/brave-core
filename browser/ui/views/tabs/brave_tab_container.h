/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_context.h"
#include "chrome/browser/ui/views/tabs/tab_container_impl.h"
#include "components/prefs/pref_member.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/scroll_view.h"

namespace views {
class ScrollView;
}  // namespace views

class BraveTabContainer : public TabContainerImpl {
  METADATA_HEADER(BraveTabContainer, TabContainerImpl)
 public:
  BraveTabContainer(TabContainerController& controller,
                    TabHoverCardController* hover_card_controller,
                    TabDragContextBase* drag_context,
                    TabSlotController& tab_slot_controller);
  ~BraveTabContainer() override;

  // Calling this will freeze this view's layout. When the returned closure
  // runs, layout will be unlocked and run immediately.
  // This is to avoid accessing invalid index during reconstruction
  // of TabContainer. In addition, we can avoid redundant layout as a side
  // effect.
  base::OnceClosure LockLayout();

  // Returns the ScrollBarMode for the scroll view used in vertical tab strip.
  views::ScrollView::ScrollBarMode GetScrollBarMode() const;

  // TabContainerImpl:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void UpdateClosingModeOnRemovedTab(int model_index, bool was_active) override;
  gfx::Rect GetTargetBoundsForClosingTab(Tab* tab,
                                         int former_model_index) const override;
  void EnterTabClosingMode(std::optional<int> override_width,
                           CloseTabSource source) override;
  bool ShouldTabBeVisible(const Tab* tab) const override;
  void StartInsertTabAnimation(int model_index) override;
  void RemoveTab(int index, bool was_active) override;
  void OnTabCloseAnimationCompleted(Tab* tab) override;
  void CompleteAnimationAndLayout() override;
  void PaintChildren(const views::PaintInfo& paint_info) override;
  void SetTabSlotVisibility() override;
  void InvalidateIdealBounds() override;
  void Layout(PassKey) override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  void OnSplitCreated(const std::vector<int>& indices) override;
  void OnSplitRemoved(const std::vector<int>& indices) override;
  void OnSplitContentsChanged(const std::vector<int>& indices) override;
  bool OnMouseWheel(const ui::MouseWheelEvent& event) override;
  void OnScrollEvent(ui::ScrollEvent* event) override;
  views::View* TargetForRect(views::View* root, const gfx::Rect& rect) override;
  bool IsPointInTab(Tab* tab,
                    const gfx::Point& point_in_tabstrip_coords) override;
  void UpdateIdealBounds() override;
  void OnTabSlotAnimationProgressed(TabSlotView* view) override;
  void SetActiveTab(std::optional<size_t> prev_active_index,
                    std::optional<size_t> new_active_index) override;
  void SetTabPinned(int model_index, TabPinned pinned) override;
  void MoveTab(int from_model_index, int to_model_index) override;

  // BrowserRootView::DropTarget
  std::optional<BrowserRootView::DropIndex> GetDropIndex(
      const ui::DropTargetEvent& event) override;
  void HandleDragUpdate(
      const std::optional<BrowserRootView::DropIndex>& index) override;
  void HandleDragExited() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest,
                           BraveTabContainerSeparator);

  class DropArrow {
   public:
    enum class Position { Vertical, Horizontal };

    DropArrow(const BrowserRootView::DropIndex& index,
              Position position,
              bool beneath,
              views::Widget* context);
    DropArrow(const DropArrow&) = delete;
    DropArrow& operator=(const DropArrow&) = delete;
    virtual ~DropArrow();

    void set_index(const BrowserRootView::DropIndex& index) { index_ = index; }
    BrowserRootView::DropIndex index() const { return index_; }

    void SetBeneath(bool beneath);
    bool beneath() const { return beneath_; }

    void SetWindowBounds(const gfx::Rect& bounds);

   private:
    // Index of the tab to drop on.
    BrowserRootView::DropIndex index_;

    Position position_ = Position::Vertical;

    bool beneath_ = false;

    // Renders the drop indicator.
    std::unique_ptr<views::Widget> arrow_window_;
    raw_ptr<views::ImageView, DanglingUntriaged> arrow_view_ = nullptr;
  };

  void UpdateLayoutOrientation();

  void PaintBoundingBoxForSplitTabs(gfx::Canvas& canvas);
  void PaintBoundingBoxForSplitTab(gfx::Canvas& canvas,
                                   const std::vector<int>& indices);

  static gfx::ImageSkia* GetDropArrowImage(
      BraveTabContainer::DropArrow::Position pos,
      bool beneath);

  void OnUnlockLayout();

  void SetDropArrow(const std::optional<BrowserRootView::DropIndex>& index);
  gfx::Rect GetDropBounds(int drop_index,
                          bool drop_before,
                          bool drop_in_group,
                          bool* is_beneath);

  void UpdateTabsBorderInSplitTab(const std::vector<int>& indices);

  // Returns the bottom y-coordinate of the pinned tabs area.
  // This is used to visible rect of unpinned tabs.
  int GetPinnedTabsAreaBottom() const;

  // Sets the scroll offset for unpinned tabs. If the offset changes, triggers
  // a layout.
  void SetScrollOffset(int offset);

  // Returns the maximum scroll offset for unpinned tabs.
  int GetMaxScrollOffset() const;

  // Returns the first and last visible unpinned slot views (tabs or group
  // headers). Returns {nullptr, nullptr} when no visible unpinned slot views
  // exist.
  std::pair<TabSlotView*, TabSlotView*> FindVisibleUnpinnedSlotViews() const;

  // Returns the ideal bounds for the given slot view (tab or group header).
  gfx::Rect GetIdealBoundsOf(TabSlotView* slot_view) const;

  // Clamp the current scroll_offset_ within valid range.
  void ClampScrollOffset();

  // Used to sets the clip path for child views (tabs, group views and etc).
  void UpdateClipPathForChildren(views::View* view,
                                 int pinned_tabs_area_bottom);

  // Updates clip path for all slot views (tabs and group views) based on
  // scroll_offset.
  void UpdateClipPathForSlotViews();

  // Update scroll offset to make the given tab visible.
  void ScrollTabToBeVisible(Tab* tab);

  // Show or hide scrollbar based on the preference
  void UpdateScrollBarVisibility();

  // Handles vertical scroll input for unpinned tabs. Returns true if the scroll
  // was handled.
  bool HandleVerticalScroll(int y_offset);

  // Updates the separator visibility and position between pinned and unpinned
  // tabs.
  void UpdatePinnedUnpinnedSeparator();

  base::flat_set<Tab*> closing_tabs_;

  raw_ptr<TabDragContextBase> drag_context_;

  // A pointer storing the global tab style to be used.
  const raw_ptr<const TabStyle> tab_style_;

  const raw_ref<TabContainerController, DanglingUntriaged> controller_;

  std::unique_ptr<DropArrow> drop_arrow_;

  BooleanPrefMember show_vertical_tabs_;
  BooleanPrefMember vertical_tabs_floating_mode_enabled_;
  BooleanPrefMember vertical_tabs_collapsed_;
  BooleanPrefMember should_show_scroll_bar_;

  bool layout_locked_ = false;

  // Size we last laid out at.
  std::optional<gfx::Size> last_layout_size_;

  // Manual vertical scroll offset for unpinned tabs. Do not manupulate this
  // value directly. Use SetScrollOffset() instead.
  int scroll_offset_ = 0;

  // Separator view between pinned and unpinned tabs
  raw_ptr<views::View> separator_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_CONTAINER_H_
