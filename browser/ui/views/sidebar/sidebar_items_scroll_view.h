/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_

#include <memory>
#include <set>

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
 public:
  explicit SidebarItemsScrollView(BraveBrowser* browser);
  ~SidebarItemsScrollView() override;

  SidebarItemsScrollView(const SidebarItemsScrollView&) = delete;
  SidebarItemsScrollView operator=(const SidebarItemsScrollView&) = delete;

  // views::View overrides:
  void Layout() override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;
  bool GetDropFormats(int* formats,
                      std::set<ui::ClipboardFormatType>* format_types) override;
  bool CanDrop(const OSExchangeData& data) override;
  int OnDragUpdated(const ui::DropTargetEvent& event) override;
  void OnDragExited() override;
  ui::mojom::DragOperation OnPerformDrop(
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
                   int index,
                   bool user_gesture) override;
  void OnItemMoved(const sidebar::SidebarItem& item, int from, int to) override;
  void OnItemRemoved(int index) override;
  void OnActiveIndexChanged(int old_index, int new_index) override;
  void OnFaviconUpdatedForItem(const sidebar::SidebarItem& item,
                               const gfx::ImageSkia& image) override;

  bool IsItemReorderingInProgress() const;
  bool IsBubbleVisible() const;

 private:
  void UpdateArrowViewsTheme();
  void UpdateArrowViewsEnabledState();
  // Return true if scroll view's area doesn't have enough bounds to show whole
  // contents view.
  bool IsScrollable() const;

  void OnButtonPressed(views::View* view);

  gfx::Rect GetTargetScrollContentsViewRectTo(bool top);
  void ScrollContentsViewBy(int offset, bool animate);

  // Returns true if |position| is in visible contents area.
  bool IsInVisibleContentsViewBounds(const gfx::Point& position) const;

  BraveBrowser* browser_ = nullptr;
  views::ImageButton* up_arrow_ = nullptr;
  views::ImageButton* down_arrow_ = nullptr;
  SidebarItemsContentsView* contents_view_ = nullptr;
  std::unique_ptr<SidebarItemDragContext> drag_context_;
  std::unique_ptr<views::BoundsAnimator> scroll_animator_for_new_item_;
  std::unique_ptr<views::BoundsAnimator> scroll_animator_for_smooth_;
  base::ScopedObservation<sidebar::SidebarModel,
                          sidebar::SidebarModel::Observer>
      model_observed_{this};
  base::ScopedMultiSourceObservation<views::BoundsAnimator,
                                     views::BoundsAnimatorObserver>
      bounds_animator_observed_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_
