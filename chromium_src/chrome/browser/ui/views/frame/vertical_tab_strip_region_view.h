// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_

#include <chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h>  // IWYU pragma: export

class VerticalTabStripRegionView
    : public VerticalTabStripRegionView_ChromiumImpl {
  METADATA_HEADER(VerticalTabStripRegionView,
                  VerticalTabStripRegionView_ChromiumImpl)

 public:
  explicit VerticalTabStripRegionView(
      tabs::VerticalTabStripStateController* state_controller,
      actions::ActionItem* root_action_item,
      BrowserView* browser_view);
  ~VerticalTabStripRegionView() override;

  void HandleMouseMoveEvent(const gfx::PointF& point_int_screen);
  // Returns true if the strip is at the leading edge in layout coordinates.
  // Since layout coordinates are mirrored in RTL, this is inverted in RTL and
  // does not name the physical side set by kVerticalTabsOnRight. Layout, resize
  // and drop-arrow code depend on this.
  bool IsVerticalTabStripLeading() const;

  // VerticalTabStripRegionView_ChromiumImpl:
  gfx::Size GetMinimumSize() const override;
  void OnAnimationProgressed(const BrowserAnimationController* controller,
                             BrowserAnimationUpdate status) override;
  void Layout(PassKey) override;
  void OnResize(int resize_amount, bool done_resizing) override;
  gfx::Point GetLinkDropArrowPosition(
      const BrowserRootView::DropIndex& drop_index,
      DropArrow::Direction* direction) override;

 private:
  void ObserveHideCompletelyPref();
  void OnHideCompletelyPrefChanged();
  bool IsMouseInHotCorner() const;

  void ObserveVerticalTabOnRightPref();
  void OnVerticalTabOnRightPrefChanged();

  PrefChangeRegistrar hide_completely_pref_registrar_;
  PrefChangeRegistrar vertical_tab_on_right_pref_registrar_;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_
