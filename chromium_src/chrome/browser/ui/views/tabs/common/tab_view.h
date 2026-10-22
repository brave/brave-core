// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_COMMON_TAB_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_COMMON_TAB_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/tabs/tab/alert_indicator_button.h"
#include "ui/views/controls/button/image_button.h"

// Adding a dummy implementation of `IsTabMuteIndicatorNotClickable`, as
// we extended AlertIndicatorButton::Delegate.
// Also adds the tree tab collapse/expand button.
#define IsApparentlyActive                                                    \
  IsTabMuteIndicatorNotClickable() override;                                  \
                                                                              \
 public:                                                                      \
  /* Updates the tree tab toggle button for the current tree tab node      */ \
  /* state.                                                                */ \
  void UpdateTreeToggleButton();                                              \
  bool ShouldShowTreeToggleButton() const;                                    \
  views::ImageButton* tree_toggle_button_for_testing() {                      \
    return tree_toggle_button_;                                               \
  }                                                                           \
                                                                              \
 private:                                                                     \
  void InitTreeToggleButton();                                                \
  void OnTreeToggleButtonPressed();                                           \
  /* Adds tree tab descendants of this tab to the selection.               */ \
  void SelectTreeTabDescendants();                                            \
  raw_ptr<views::ImageButton> tree_toggle_button_ = nullptr;                  \
  bool IsApparentlyActive

#include <chrome/browser/ui/views/tabs/common/tab_view.h>  // IWYU pragma: export

#undef IsApparentlyActive

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_COMMON_TAB_VIEW_H_
