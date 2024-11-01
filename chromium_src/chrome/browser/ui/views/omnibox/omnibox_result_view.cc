/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/omnibox/omnibox_result_view.h"

#include "chrome/browser/ui/views/omnibox/omnibox_match_cell_view.h"
#include "ui/views/background.h"

// We want the rounded rect background to have a radius of 0
#define kRowHeight kRowHeight * 0

#include "src/chrome/browser/ui/views/omnibox/omnibox_result_view.cc"

#undef kRowHeight
