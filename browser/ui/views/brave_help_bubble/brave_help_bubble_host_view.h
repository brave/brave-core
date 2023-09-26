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
#include "ui/views/widget/widget_observer.h"

class BraveHelpBubbleHostView : public views::View,
                                public views::ViewObserver,
                                public views::WidgetObserver {
 public:
  METADATA_HEADER(BraveHelpBubbleHostView);

  BraveHelpBubbleHostView();
  BraveHelpBubbleHostView(const BraveHelpBubbleHostView&) = delete;
  BraveHelpBubbleHostView& operator=(const BraveHelpBubbleHostView&) = delete;
  ~BraveHelpBubbleHostView() override;

  // Return true when bubble is shown.
  bool Show();
  void Hide();

  void set_text(const std::string& text) { text_ = text; }
  void set_tracked_element(views::View* element) { tracked_element_ = element; }

 private:
  void UpdatePosition();

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;

  // views::ViewObserver:
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewIsDeleting(views::View* observed_view) override;
  void OnViewVisibilityChanged(views::View* observed_view,
                               views::View* starting_view) override;

  // views::WidgetObserver:
  void OnWidgetBoundsChanged(views::Widget* widget, const gfx::Rect&) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  void OnTrackedElementActivated(ui::TrackedElement* element);

  std::string text_;
  raw_ptr<views::View> tracked_element_ = nullptr;
  raw_ptr<views::Widget> help_bubble_ = nullptr;
  ui::ElementTracker::Subscription activated_subscription_;
  base::ScopedObservation<views::View, views::ViewObserver>
      tracked_view_observation_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      host_widget_observation_{this};
  base::ScopedObservation<views::Widget, views::WidgetObserver>
      bubble_widget_observation_{this};

  base::WeakPtrFactory<BraveHelpBubbleHostView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_BRAVE_HELP_BUBBLE_HOST_VIEW_H_
