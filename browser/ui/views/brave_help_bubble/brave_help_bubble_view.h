// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_BRAVE_HELP_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_BRAVE_HELP_BUBBLE_VIEW_H_

#include <memory>

#include "base/timer/timer.h"
#include "brave/browser/ui/views/brave_help_bubble/brave_help_bubble_delegate.h"
#include "chrome/browser/ui/browser.h"
#include "ui/views/interaction/element_tracker_views.h"
#include "ui/views/view.h"

class BraveHelpBubbleView : public views::View,
                            public BraveHelpBubbleDelegate::Observer,
                            public views::ViewObserver {
 public:
  METADATA_HEADER(BraveHelpBubbleView);

  explicit BraveHelpBubbleView(View* tracked_element);
  BraveHelpBubbleView(const BraveHelpBubbleView&) = delete;
  BraveHelpBubbleView& operator=(const BraveHelpBubbleView&) = delete;
  ~BraveHelpBubbleView() override;

  void Show();
  void Hide();
  static base::WeakPtr<BraveHelpBubbleView> Create(View* tracked_element,
                                                   const std::u16string text);

 private:
  void InitElementTrackers();
  void UpdatePosition();
  // views::View:
  void AddedToWidget() override;
  void OnPaint(gfx::Canvas* canvas) override;
  // BraveHelpBubbleDelegate::Observer:
  void OnBubbleClosing(Widget* widget) override;

  void OnTrackedElementShown(ui::TrackedElement* element);
  void OnTrackedElementHidden(ui::TrackedElement* element);
  void OnViewBoundsChanged(views::View* observed_view) override;

  std::u16string text_;
  raw_ptr<views::View> tracked_element_;
  raw_ptr<BraveHelpBubbleDelegate> brave_help_bubble_delegate_ = nullptr;
  ui::ElementContext context_;
  ui::ElementTracker::Subscription shown_subscription_;
  ui::ElementTracker::Subscription hidden_subscription_;
  base::ScopedObservation<views::View, views::ViewObserver> scoped_observation_{
      this};
  base::WeakPtrFactory<BraveHelpBubbleView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_BRAVE_HELP_BUBBLE_VIEW_H_
