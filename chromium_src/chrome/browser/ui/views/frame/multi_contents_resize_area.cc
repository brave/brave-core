/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/multi_contents_view.h"

// Reset resize area position when double clicked instead of swap.
#define OnSwap ResetResizeArea

#include <chrome/browser/ui/views/frame/multi_contents_resize_area.cc>

#undef OnSwap
