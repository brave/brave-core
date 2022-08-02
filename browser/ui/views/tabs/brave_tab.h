// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_

#include <string>

#include "chrome/browser/ui/views/tabs/tab.h"
#include "ui/gfx/geometry/point.h"

class BraveTab : public Tab {
 public:
  using Tab::Tab;
  BraveTab(const BraveTab&) = delete;
  BraveTab& operator=(const BraveTab&) = delete;
  ~BraveTab() override;

  std::u16string GetTooltipText(const gfx::Point& p) const override;

  // Overridden because we moved alert button to left side in the tab whereas
  // upstream put it on right side. Need to consider this change for calculating
  // largest selectable region.
  int GetWidthOfLargestSelectableRegion() const override;

  void ActiveStateChanged() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_H_
