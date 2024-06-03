/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/layout_constants.h"

// Change button's raidus directly instead of changing TOOLBAR_CORNER_RADIUS
// constants from our brave_layout_constants.cc because we already override
// TOOLBAR_CORNER_RADIUS with different value.
// Not re-defined SetCornerRadius() like below because it is declared from many
// headers. #define SetCornerRadius(...) SetCornerRadius(8)
#define TOOLBAR_CORNER_RADIUS TOOLBAR_CORNER_RADIUS)); (SetCornerRadius(8

#include "src/chrome/browser/ui/views/omnibox/omnibox_suggestion_button_row_view.cc"

#undef TOOLBAR_CORNER_RADIUS
