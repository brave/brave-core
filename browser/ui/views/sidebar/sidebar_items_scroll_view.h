/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_

#include <memory>

#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "ui/views/animation/bounds_animator.h"
#include "ui/views/animation/bounds_animator_observer.h"
#include "ui/views/view.h"

namespace views {
class BoundsAnimator;
class ImageButton;
}  // namespace views

class BraveBrowser;
class SidebarItemsContentsView;

class SidebarItemsScrollView : public views::View,
                               public views::BoundsAnimatorObserver,
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

  // views::BoundsAnimatorObserver overrides:
  void OnBoundsAnimatorProgressed(views::BoundsAnimator* animator) override;
  void OnBoundsAnimatorDone(views::BoundsAnimator* animator) override;

  // sidebar::SidebarModel::Observer overrides:
  void OnItemAdded(const sidebar::SidebarItem& item,
                   int index,
                   bool user_gesture) override;
  void OnItemRemoved(int index) override;
  void OnActiveIndexChanged(int old_index, int new_index) override;
  void OnFaviconUpdatedForItem(const sidebar::SidebarItem& item,
                               const gfx::ImageSkia& image) override;

 private:
  void UpdateArrowViewsTheme();
  void UpdateArrowViewsEnabledState();
  // Return true if scroll view's area doesn't have enough bounds to show whole
  // contents view.
  bool IsScrollable() const;

  void OnButtonPressed(views::View* view);

  gfx::Rect GetTargetScrollContentsViewRectTo(bool top);
  void ScrollContentsViewBy(int offset, bool animate);

  BraveBrowser* browser_ = nullptr;
  views::ImageButton* up_arrow_ = nullptr;
  views::ImageButton* down_arrow_ = nullptr;
  SidebarItemsContentsView* contents_view_ = nullptr;
  std::unique_ptr<views::BoundsAnimator> scroll_animator_for_new_item_;
  std::unique_ptr<views::BoundsAnimator> scroll_animator_for_smooth_;
  ScopedObserver<sidebar::SidebarModel, sidebar::SidebarModel::Observer>
      model_observed_{this};
  ScopedObserver<views::BoundsAnimator, views::BoundsAnimatorObserver>
      bounds_animator_observed_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEMS_SCROLL_VIEW_H_
