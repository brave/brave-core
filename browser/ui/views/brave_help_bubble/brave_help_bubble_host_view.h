// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_BRAVE_HELP_BUBBLE_HOST_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_BRAVE_HELP_BUBBLE_HOST_VIEW_H_

#include <memory>
#include <string>

#include "brave/browser/ui/views/brave_help_bubble/brave_help_bubble_delegate_view.h"
#include "ui/views/interaction/element_tracker_views.h"
#include "ui/views/view.h"

class BraveHelpBubbleHostView : public views::View,
                                public BraveHelpBubbleDelegateView::Observer,
                                public views::ViewObserver {
 public:
  METADATA_HEADER(BraveHelpBubbleHostView);

  BraveHelpBubbleHostView(const BraveHelpBubbleHostView&) = delete;
  BraveHelpBubbleHostView& operator=(const BraveHelpBubbleHostView&) = delete;
  ~BraveHelpBubbleHostView() override;

  static base::WeakPtr<BraveHelpBubbleHostView> Create(View* tracked_element,
                                                       const std::string text);
  void Show();
  void Hide();

 private:
  explicit BraveHelpBubbleHostView(View* tracked_element);

  void InitElementTrackers();
  void UpdatePosition();
  void StartObservingAndShow();

  // views::View:
  void AddedToWidget() override;
  void OnPaint(gfx::Canvas* canvas) override;

  // BraveHelpBubbleDelegateView::Observer:
  void OnBubbleDestroying(views::Widget* widget) override;
  // views::ViewObserver:
  void OnViewBoundsChanged(views::View* observed_view) override;

  void OnTrackedElementShown(ui::TrackedElement* element);
  void OnTrackedElementHidden(ui::TrackedElement* element);

  std::string text_;
  raw_ptr<views::View> tracked_element_;
  raw_ptr<BraveHelpBubbleDelegateView> brave_help_bubble_delegate_view_ =
      nullptr;
  ui::ElementContext context_;
  ui::ElementTracker::Subscription shown_subscription_;
  ui::ElementTracker::Subscription hidden_subscription_;
  base::ScopedObservation<BraveHelpBubbleDelegateView,
                          BraveHelpBubbleDelegateView::Observer>
      bubble_help_delegate_observation_{this};
  base::ScopedObservation<views::View, views::ViewObserver> view_observation_{
      this};
  base::WeakPtrFactory<BraveHelpBubbleHostView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_BRAVE_HELP_BUBBLE_HOST_VIEW_H_
