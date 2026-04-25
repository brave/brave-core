/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_SPLIT_TABS_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_SPLIT_TABS_BUTTON_H_

class SplitTabsToolbarButton;

using BraveSplitTabsToolbarButton = SplitTabsToolbarButton;

#define GetIconsForTesting(...)       \
  GetIconsForTesting(__VA_ARGS__);    \
  friend BraveSplitTabsToolbarButton; \
  FRIEND_TEST_ALL_PREFIXES(BraveToolbarViewTest, SplitTabsToolbarButtonTest)

#define SplitTabsToolbarButton SplitTabsToolbarButton_ChromiumImpl

#include <chrome/browser/ui/views/toolbar/split_tabs_button.h>  // IWYU pragma: export

#undef SplitTabsToolbarButton

#undef GetIconsForTesting

class SplitTabsToolbarButton : public SplitTabsToolbarButton_ChromiumImpl {
  METADATA_HEADER(SplitTabsToolbarButton, SplitTabsToolbarButton_ChromiumImpl)

 public:
  explicit SplitTabsToolbarButton(Browser* browser);
  ~SplitTabsToolbarButton() override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TOOLBAR_SPLIT_TABS_BUTTON_H_
