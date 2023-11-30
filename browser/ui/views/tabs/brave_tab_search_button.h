/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_SEARCH_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_SEARCH_BUTTON_H_

#include "chrome/browser/ui/views/tabs/tab_search_button.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/gfx/geometry/size.h"

class BraveTabSearchButton : public TabSearchButton {
 public:
  METADATA_HEADER(BraveTabSearchButton);

  explicit BraveTabSearchButton(TabStripController* tab_strip_controller,
                                Edge flat_edge);
  ~BraveTabSearchButton() override;
  BraveTabSearchButton(const BraveTabSearchButton&) = delete;
  BraveTabSearchButton& operator=(const BraveTabSearchButton&) = delete;

  void SetBubbleArrow(views::BubbleBorder::Arrow arrow);

  // TabSearchButton:
  void UpdateColors() override;
  gfx::Size CalculatePreferredSize() const override;
  int GetCornerRadius() const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_SEARCH_BUTTON_H_
