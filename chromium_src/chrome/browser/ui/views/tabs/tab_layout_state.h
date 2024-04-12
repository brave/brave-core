/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_LAYOUT_STATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_LAYOUT_STATE_H_

enum class TabTiledState { kNone, kFirst, kSecond };

#define IsClosed                                     \
  IsClosed_Unused() const {                          \
    return false;                                    \
  }                                                  \
                                                     \
 public:                                             \
  void set_tiled_state(TabTiledState tiled_state) {  \
    tiled_state_ = tiled_state;                      \
  }                                                  \
  TabTiledState tiled_state() const {                \
    return tiled_state_;                             \
  }                                                  \
                                                     \
 private:                                            \
  TabTiledState tiled_state_ = TabTiledState::kNone; \
                                                     \
 public:                                             \
  bool IsClosed

#include "src/chrome/browser/ui/views/tabs/tab_layout_state.h"  // IWYU pragma: export

#undef IsClosed

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_LAYOUT_STATE_H_
