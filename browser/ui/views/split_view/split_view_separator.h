/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_SEPARATOR_H_
#define BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_SEPARATOR_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/split_view/split_view_separator_delegate.h"
#include "ui/views/controls/resize_area.h"
#include "ui/views/controls/resize_area_delegate.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

class Browser;

// A separator view that is located between contents web views in BrowserView.
// This separator is used to resize the contents web views.
class SplitViewSeparator : public views::ResizeArea,
                           public views::ResizeAreaDelegate,
                           public views::WidgetObserver,
                           public views::ViewObserver {
  METADATA_HEADER(SplitViewSeparator, views::ResizeArea)
 public:
  explicit SplitViewSeparator(Browser* browser);
  ~SplitViewSeparator() override;

  void set_delegate(SplitViewSeparatorDelegate* delegate) {
    resize_area_delegate_ = delegate;
  }

  void SetOrientation(SplitViewBrowserData::Orientation orientation);
  auto orientation() const { return orientation_; }

  // views::View:
  void AddedToWidget() override;
  void VisibilityChanged(views::View* starting_from, bool is_visible) override;
  ui::Cursor GetCursor(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseCaptureLost() override;
  void Layout(PassKey) override;
  void ViewHierarchyChanged(
      const views::ViewHierarchyChangedDetails& details) override;

  // views::ResizeAreaDelegate:
  void OnResize(int resize_amount, bool done_resizing) override;

  // views::WidgetObserver:
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

  // views::ViewObserver:
  void OnViewBoundsChanged(views::View* observed_view) override;

 private:
  void CreateMenuButton();
  void LayoutMenuButton();

  ui::Cursor GetCursor();

  template <class Event>
  auto ConvertYToScreen(const Event& event) {
    return ConvertPointToScreen(this, gfx::Point(0, event.y())).y();
  }

  template <class Event>
  void SetInitialPosition(const Event& event) {
    CHECK(orientation_ == SplitViewBrowserData::Orientation::kHorizontal);
    initial_y_position_in_screen_ = ConvertYToScreen(event);
  }

  raw_ptr<Browser> browser_ = nullptr;

  raw_ptr<SplitViewSeparatorDelegate> resize_area_delegate_ = nullptr;

  raw_ptr<views::Widget> menu_button_widget_ = nullptr;

  SplitViewBrowserData::Orientation orientation_ =
      SplitViewBrowserData::Orientation::kVertical;

  int initial_y_position_in_screen_ = 0;

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      parent_widget_observation_{this};

  base::ScopedObservation<views::View, views::ViewObserver>
      parent_view_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_SEPARATOR_H_
