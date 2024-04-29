/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_

#include <memory>
#include <optional>
#include <set>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/views/animation/bounds_animator.h"
#include "ui/views/animation/bounds_animator_observer.h"
#include "ui/views/drag_controller.h"
#include "ui/views/view.h"

namespace views {
class BoundsAnimator;
class ImageButton;
}  // namespace views

namespace sidebar {
class SidebarBrowserTest;
}  // namespace sidebar

class BraveBrowser;
class SidebarItemDragContext;
class SidebarItemsContentsView;

// This view includes sidebar items contents view. If this view has sufficient
// bounds to show all of items contents view, this scroll view's size is same
// as items contents view. Otherwise, this view shows part of items contents
// view and items view's visible area is controlled by up/down arrow button.
class SidebarItemsScrollView : public views::View,
                               public views::BoundsAnimatorObserver,
                               public views::DragController,
                               public sidebar::SidebarModel::Observer {
  METADATA_HEADER(SidebarItemsScrollView, views::View)
 public:
  explicit SidebarItemsScrollView(BraveBrowser* browser);
  ~SidebarItemsScrollView() override;

  SidebarItemsScrollView(const SidebarItemsScrollView&) = delete;
  SidebarItemsScrollView operator=(const SidebarItemsScrollView&) = delete;

  // views::View overrides:
  void Layout(PassKey) override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  bool GetDropFormats(int* formats,
                      std::set<ui::ClipboardFormatType>* format_types) override;
  bool CanDrop(const OSExchangeData& data) override;
  void OnDragExited() override;
  int OnDragUpdated(const ui::DropTargetEvent& event) override;
  views::View::DropCallback GetDropCallback(
      const ui::DropTargetEvent& event) override;

  // views::BoundsAnimatorObserver overrides:
  void OnBoundsAnimatorProgressed(views::BoundsAnimator* animator) override;
  void OnBoundsAnimatorDone(views::BoundsAnimator* animator) override;

  // views::DragController overrides:
  void WriteDragDataForView(views::View* sender,
                            const gfx::Point& press_pt,
                            ui::OSExchangeData* data) override;
  int GetDragOperationsForView(views::View* sender,
                               const gfx::Point& p) override;
  bool CanStartDragForView(views::View* sender,
                           const gfx::Point& press_pt,
                           const gfx::Point& p) override;

  // sidebar::SidebarModel::Observer overrides:
  void OnItemAdded(const sidebar::SidebarItem& item,
                   size_t index,
                   bool user_gesture) override;
  void OnItemMoved(const sidebar::SidebarItem& item,
                   size_t from,
                   size_t to) override;
  void OnItemRemoved(size_t index) override;
  void OnActiveIndexChanged(std::optional<size_t> old_index,
                            std::optional<size_t> new_index) override;
  void OnItemUpdated(const sidebar::SidebarItem& item,
                     const sidebar::SidebarItemUpdate& update) override;
  void OnFaviconUpdatedForItem(const sidebar::SidebarItem& item,
                               const gfx::ImageSkia& image) override;

  bool IsItemReorderingInProgress() const;
  bool IsBubbleVisible() const;
  void Update();

 private:
  friend class sidebar::SidebarBrowserTest;

  void UpdateArrowViewsTheme();
  void UpdateArrowViewsEnabledState();
  // Return true if scroll view's area doesn't have enough bounds to show whole
  // contents view.
  bool IsScrollable() const;

  void OnButtonPressed(views::View* view);

  gfx::Rect GetTargetScrollContentsViewRectTo(bool top);
  void ScrollContentsViewBy(int offset, bool animate);

  // Returns true when needs scroll for showing an item at |index|.
  bool NeedScrollForItemAt(size_t index) const;

  // Get bounds for |contents_view_| to make item at |index| visible in
  // scroll view.
  gfx::Rect GetTargetScrollContentsViewRectForItemAt(size_t index) const;

  // Put NOLINT here because our cpp linter complains -
  // "make const or use a pointer: ui::mojom::DragOperation& output_drag_op"
  // But can't avoid because View::DropCallback uses non const refererence
  // as its parameter type.
  void PerformDrop(const ui::DropTargetEvent& event,
                   ui::mojom::DragOperation& output_drag_op,
                   std::unique_ptr<ui::LayerTreeOwner> drag_image_layer_owner);

  // Returns true if |position| is in visible contents area.
  bool IsInVisibleContentsViewBounds(const gfx::Point& position) const;
  void ClearDragIndicator();

  std::optional<size_t> lastly_added_item_index_;
  raw_ptr<BraveBrowser> browser_ = nullptr;
  raw_ptr<views::ImageButton> up_arrow_ = nullptr;
  raw_ptr<views::ImageButton> down_arrow_ = nullptr;
  raw_ptr<SidebarItemsContentsView> contents_view_ = nullptr;
  std::unique_ptr<SidebarItemDragContext> drag_context_;
  std::unique_ptr<views::BoundsAnimator> scroll_animator_for_item_;
  std::unique_ptr<views::BoundsAnimator> scroll_animator_for_smooth_;
  base::ScopedObservation<sidebar::SidebarModel,
                          sidebar::SidebarModel::Observer>
      model_observed_{this};
  base::ScopedMultiSourceObservation<views::BoundsAnimator,
                                     views::BoundsAnimatorObserver>
      bounds_animator_observed_{this};
  base::WeakPtrFactory<SidebarItemsScrollView> weak_ptr_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_
