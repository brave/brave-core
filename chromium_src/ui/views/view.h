/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_VIEW_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_VIEW_H_

class SharedPinnedTabDummyView;

// DO NOT ADD TO THIS LIST!
// These existing cases are "grandfathered in", but there shouldn't be more.
// See comments atop View class in ui/views/view.h.
#define BRAVE_VIEW_OWNED_BY_CLIENT_PASS_KEY \
  friend class ::SharedPinnedTabDummyView;

#include <ui/views/view.h>  // IWYU pragma: export

#undef BRAVE_VIEW_OWNED_BY_CLIENT_PASS_KEY

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_VIEW_H_
