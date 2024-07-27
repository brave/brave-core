/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// We always want the tab search button to render after the tab strip (i.e., on
// the right side of the frame), regardless of platform.
#define BRAVE_GET_DEFAULT_TAB_SEARCH_RIGHT_ALIGNED \
  return true;
#include "src/chrome/browser/ui/tabs/tab_strip_prefs.cc"
#undef BRAVE_GET_DEFAULT_TAB_SEARCH_RIGHT_ALIGNED
