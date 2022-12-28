/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_

#include "chrome/browser/ui/views/tabs/tab_group_header.h"

namespace tab_groups {
class TabGroupId;
}  // namespace tab_groups

class BraveTabGroupHeader : public TabGroupHeader {
 public:
  METADATA_HEADER(BraveTabGroupHeader);

  constexpr static int kPaddingForGroup = 4;

  static SkColor GetDarkerColorForGroup(const tab_groups::TabGroupId& group_id,
                                        TabSlotController* controller,
                                        bool dark_mode);

  using TabGroupHeader::TabGroupHeader;
  ~BraveTabGroupHeader() override;

  // TabGroupHeader:
  void VisualsChanged() override;
  void Layout() override;

 private:
  void LayoutTitleChip();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_
