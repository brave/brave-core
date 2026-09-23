// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MODEL_H_

// Add accessor for opener_was_set_for_empty_new_tab. This is used for tree tabs
// to check if the opener was set for an empty new tab in
// TabStripModel::AddTab().
#define set_opener(...)                           \
  set_opener_was_set_for_empty_new_tab() {        \
    opener_was_set_for_empty_new_tab_ = true;     \
  }                                               \
  bool opener_was_set_for_empty_new_tab() const { \
    return opener_was_set_for_empty_new_tab_;     \
  }                                               \
                                                  \
 private:                                         \
  bool opener_was_set_for_empty_new_tab_ = false; \
                                                  \
 public:                                          \
  void set_opener(__VA_ARGS__)

#include <chrome/browser/ui/tabs/tab_model.h>  // IWYU pragma: export
#undef set_opener

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MODEL_H_
