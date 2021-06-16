/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_ADDED_FEEDBACK_BUBBLE_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_ADDED_FEEDBACK_BUBBLE_H_

#include <memory>

#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/linear_animation.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/view_observer.h"

// Observes SidebarItemsContentsView's bounds to locate feedback bubble
// properly. Passed |anchor_view| is newly added item and its bound is not
// changed when it's moved. The reason is SidebarItemsScrollView moves up and
// down whole SidebarItemsContentsView. So, observes SidebarItemsContentsView to
// put feedback bubble next to the new item always.
class SidebarItemAddedFeedbackBubble : public views::BubbleDialogDelegateView,
                                       public views::ViewObserver,
                                       public gfx::AnimationDelegate {
 public:
  SidebarItemAddedFeedbackBubble(views::View* anchor_view,
                                 views::View* items_contents_view);
  ~SidebarItemAddedFeedbackBubble() override;

  SidebarItemAddedFeedbackBubble(const SidebarItemAddedFeedbackBubble&) =
      delete;
  SidebarItemAddedFeedbackBubble& operator=(
      const SidebarItemAddedFeedbackBubble) = delete;

  // views::BubbleDialogDelegateView overrides:
  void OnWidgetVisibilityChanged(views::Widget* widget, bool visible) override;
  void OnWidgetDestroying(views::Widget* widget) override;
  std::unique_ptr<views::NonClientFrameView> CreateNonClientFrameView(
      views::Widget* widget) override;

  // AnimationDelegate implementations:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

  // views::ViewObserver overrides:
  void OnViewBoundsChanged(View* observed_view) override;

 private:
  void AddChildViews();

  // If this timer is fired, this will start to fade.
  base::OneShotTimer fade_timer_;
  gfx::LinearAnimation animation_;

  base::ScopedObservation<views::View, views::ViewObserver> observed_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_ITEM_ADDED_FEEDBACK_BUBBLE_H_
