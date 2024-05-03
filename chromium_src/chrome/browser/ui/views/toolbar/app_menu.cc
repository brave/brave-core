/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/toolbar/app_menu.h"

// Upstream uses wrong api for setting color. It comes from
// https://chromium-review.googlesource.com/c/chromium/src/+/4395705
#define SetTextColor SetTextColorId

#include "src/chrome/browser/ui/views/toolbar/app_menu.cc"

#undef SetTextColor
