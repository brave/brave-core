// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_DISPATCH_MATCHED_TAB_INFO_H_
#define BRAVE_BROWSER_UI_GESTURES_DISPATCH_MATCHED_TAB_INFO_H_

#include <vector>

#include "base/memory/raw_ref.h"

class BrowserWindowInterface;

namespace gestures {

// A single open tab, identified by its window and index within that
// window's tab strip.
struct MatchedTabInfo final {
  raw_ref<BrowserWindowInterface> browser_window;
  int index = 0;
};

// Every tab matching a search, in `kCreation` order.
using MatchedTabs = std::vector<MatchedTabInfo>;

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_DISPATCH_MATCHED_TAB_INFO_H_
