/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/omnibox/omnibox_result_view.h"

#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/views/background.h"

#define CreateThemedRoundedRectBackground(...) \
  CreateThemedRoundedRectBackground(GetOmniboxBackgroundColorId(part_state), 0)

// For some reason since a recent change the unused variable warning started
// being triggered for `radii`. It is unfortunate that just tagging
// `[[maybe_unused]]` after the type is not allowed, so a dummy is necesary that
// can be ignored in a less than conventional way, and then the type can be
// tagged as `[[maybe_unused]]`.
#define radii             \
  dummy = {};             \
  if (!dummy.IsEmpty()) { \
  }                       \
  [[maybe_unused]] gfx::RoundedCornersF radii

#include "src/chrome/browser/ui/views/omnibox/omnibox_result_view.cc"

#undef CreateThemedRoundedRectBackground
#undef radii
