/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TABS_VERTICAL_TAB_STRIP_WIDGET_DELEGATE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TABS_VERTICAL_TAB_STRIP_WIDGET_DELEGATE_VIEW_H_

#include <memory>

#include "base/scoped_observation.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

class BrowserView;
class BraveVerticalTabStripRegionView;

// This class wraps BraveVerticalTabStripRegionView and show them atop a Widget.
// Vertical tab strip could be overlaps with contents web view and
// we need a Widget to accept user events ahead of contents web view.
// This Widget's coordinates and visibility are synchronized with a host view
// given by Create() method. The client of this class should attach this
// to parent widget, typically BrowserView/Frame. An then this widget will be a
// child of BrowserView's Widget with Control widget type.
//
// Usage:
//  auto* host_view = AddChildView(std::make_unique<views::View>());
//  VerticalTabStripWidgetDelegateView::Create(browser_view, host_view);
//  host_view->SetVisible(true);  // will show up the widget
//  host_view->SetBounds(gfx::Rect(0, 0, 100, 100));  // will layout the widget.
//                                                    // But size could be
//                                                    // different based on
//                                                    // state.
//
class VerticalTabStripWidgetDelegateView : public views::View,
                                           public views::ViewObserver {
  METADATA_HEADER(VerticalTabStripWidgetDelegateView, views::View)
 public:
  VerticalTabStripWidgetDelegateView(BrowserView* browser_view,
                                     views::View* host);
  ~VerticalTabStripWidgetDelegateView() override;

  BraveVerticalTabStripRegionView* vertical_tab_strip_region_view() const {
    return region_view_;
  }

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override;

  // views::ViewObserver:
  void OnViewVisibilityChanged(views::View* observed_view,
                               views::View* starting_view,
                               bool visible) override;
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnViewIsDeleting(views::View* observed_view) override;

 private:

  void UpdateVerticalTabBounds();

  raw_ptr<BrowserView> browser_view_ = nullptr;
  raw_ptr<views::View> host_ = nullptr;
  raw_ptr<BraveVerticalTabStripRegionView> region_view_ = nullptr;

  base::ScopedObservation<views::View, views::ViewObserver>
      host_view_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TABS_VERTICAL_TAB_STRIP_WIDGET_DELEGATE_VIEW_H_
