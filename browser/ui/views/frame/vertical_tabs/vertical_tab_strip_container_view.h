/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TABS_VERTICAL_TAB_STRIP_CONTAINER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TABS_VERTICAL_TAB_STRIP_CONTAINER_VIEW_H_

#include "base/scoped_observation.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

class BrowserView;
class BraveVerticalTabStripRegionView;

// A layer-backed view, sibling to |host_| inside BraveBrowserView, that wraps
// BraveVerticalTabStripRegionView. Its bounds are derived from the observed
// |host_|: same origin (left-aligned) or right edge (right-aligned), but its
// width tracks the region view's preferred width rather than the host's width.
// This lets the strip expand beyond the host and overlap the web contents in
// floating mode. SetPaintToLayer() ensures it renders above the contents view
// in that case.
class BraveVerticalTabStripContainerView : public views::View,
                                           public views::ViewObserver {
  METADATA_HEADER(BraveVerticalTabStripContainerView, views::View)
 public:
  BraveVerticalTabStripContainerView(BrowserView* browser_view,
                                     views::View* host);
  ~BraveVerticalTabStripContainerView() override;

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

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TABS_VERTICAL_TAB_STRIP_CONTAINER_VIEW_H_
