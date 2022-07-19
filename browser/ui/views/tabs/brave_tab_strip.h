/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_

#include "chrome/browser/ui/views/tabs/tab_strip.h"

class BraveTabStrip : public TabStrip {
 public:
  using TabStrip::TabStrip;
  ~BraveTabStrip() override;
  BraveTabStrip(const BraveTabStrip&) = delete;
  BraveTabStrip& operator=(const BraveTabStrip&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(ColorPaletteTest, LightThemeMinimumContrast);

  // TabStrip overrides:
  SkColor GetTabSeparatorColor() const override;
  bool ShouldDrawStrokes() const override;

  // Exposed for testing.
  static constexpr float kBraveMinimumContrastRatioForOutlines = 1.2797f;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_
