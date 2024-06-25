/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ui/views/layout/layout_provider.h"

// Set our radius value directly as kOmniboxExpandedRadius is mapped to
// more general token ShapeSysTokens::kMedium.
#define kOmniboxExpandedRadius kOmniboxExpandedRadius); (corner_radius = 4

#include "src/chrome/browser/ui/views/omnibox/rounded_omnibox_results_frame.cc"

#undef kOmniboxExpandedRadius

// static
int RoundedOmniboxResultsFrame::GetShadowElevation() {
  // Expose a constant defined in cc file.
  return kElevation;
}
