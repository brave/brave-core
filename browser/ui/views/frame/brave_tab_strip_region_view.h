/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_VIEW_H_

#include "base/callback_list.h"
#include "chrome/browser/ui/views/frame/horizontal_tab_strip_region_view.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"

class TabStripControlButton;

// `HorizontalTabStripRegionView` is customized to insert a medium-sized gap
// before the first tab when the browser frame is not fullscreen or maximized.
// Optional horizontal scroll buttons are laid out as:
// [leading scroll] [tab strip] [trailing scroll] [gap] [new tab button].
class BraveHorizontalTabStripRegionView : public HorizontalTabStripRegionView {
  METADATA_HEADER(BraveHorizontalTabStripRegionView,
                  HorizontalTabStripRegionView)

 public:
  template <typename... Args>
  explicit BraveHorizontalTabStripRegionView(Args&&... args)
      : HorizontalTabStripRegionView(std::forward<Args>(args)...) {
    Initialize();
  }

  ~BraveHorizontalTabStripRegionView() override;

  void Layout(PassKey) override;
  views::View::Views GetChildrenInZOrder() override;

  TabStripControlButton* tab_scroll_previous_for_testing() {
    return tab_scroll_previous_button_;
  }  // IN-TEST

  TabStripControlButton* tab_scroll_next_for_testing() {
    return tab_scroll_next_button_;
  }  // IN-TEST

 private:
  // TabStripRegionView:
  void UpdateTabStripMargin() override;
  void OnDragEntered(const ui::DropTargetEvent& event) override;
  int OnDragUpdated(const ui::DropTargetEvent& event) override;

  void Initialize();
  void CreateScrollButtonsIfNeeded();
  void UpdateScrollButtonsVisibility();
  void OnShowHorizontalTabScrollButtonsChanged();
  void UpdateTrailingScrollButtonMargin(bool vertical_tabs);
  void OnScrollPreviousPressed();
  void OnScrollNextPressed();
  bool HaveScrollButtons() const;

  raw_ptr<TabStripControlButton> tab_scroll_previous_button_ = nullptr;
  raw_ptr<TabStripControlButton> tab_scroll_next_button_ = nullptr;

  base::CallbackListSubscription horizontal_scroll_offset_changed_subscription_;

  BooleanPrefMember show_horizontal_tab_scroll_buttons_;

  base::WeakPtrFactory<BraveHorizontalTabStripRegionView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_VIEW_H_
