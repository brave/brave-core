/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_

#include <optional>

#include "chrome/browser/ui/views/tabs/tab_group_header.h"

namespace tab_groups {
class TabGroupId;
}  // namespace tab_groups

class BraveTabGroupHeader : public TabGroupHeader {
 public:
  METADATA_HEADER(BraveTabGroupHeader);

  constexpr static int kPaddingForGroup = 4;
  constexpr static int kIconSize = 16;

  BraveTabGroupHeader(TabSlotController& tab_slot_controller,
                      const tab_groups::TabGroupId& group,
                      const TabGroupStyle& style);
  BraveTabGroupHeader(const BraveTabGroupHeader&) = delete;
  BraveTabGroupHeader& operator=(const BraveTabGroupHeader&) = delete;

  ~BraveTabGroupHeader() override;

  // TabGroupHeader:
  void AddedToWidget() override;
  void VisualsChanged() override;
  int GetDesiredWidth() const override;
  void Layout() override;

  void AddNewTab();

 private:
  bool ShouldShowVerticalTabs() const;
  void LayoutTitleChipForVerticalTabs();
  SkColor GetGroupColor() const;
  std::optional<SkColor> GetChipBackgroundColor() const;

  raw_ptr<views::ImageButton> plus_icon_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_
