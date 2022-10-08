/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_

#include "chrome/browser/ui/views/tabs/tab_group_header.h"

class BraveTabGroupHeader : public TabGroupHeader {
 public:
  METADATA_HEADER(BraveTabGroupHeader);

  static int GetLeftPaddingForVerticalTabs();

  using TabGroupHeader::TabGroupHeader;
  ~BraveTabGroupHeader() override;

  // TabGroupHeader:
  void VisualsChanged() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HEADER_H_
