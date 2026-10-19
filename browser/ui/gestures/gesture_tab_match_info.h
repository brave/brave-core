// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_TAB_MATCH_INFO_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_TAB_MATCH_INFO_H_

#include "base/memory/raw_ptr.h"

class BrowserWindowInterface;

namespace gestures {

// A single open tab, identified by its window and index within that
// window's tab strip.
struct TabMatchInfo {
  raw_ptr<BrowserWindowInterface> browser_window;
  int tab_index = 0;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_TAB_MATCH_INFO_H_
