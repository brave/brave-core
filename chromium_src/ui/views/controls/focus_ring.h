// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_FOCUS_RING_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_FOCUS_RING_H_

#define RefreshLayer                              \
  RefreshLayer_UnUsed() {}                        \
  SkColor GetColor(View* focus_ring, bool valid); \
  void RefreshLayer

#include "../../../../../ui/views/controls/focus_ring.h"

#undef RefreshLayer

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_FOCUS_RING_H_
