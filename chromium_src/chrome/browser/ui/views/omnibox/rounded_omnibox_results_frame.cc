/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/omnibox/rounded_omnibox_results_frame.h"

#include "base/check_op.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/views/layout/layout_provider.h"

// Replacing for `ShapeContextTokensOverride` to select the override call for
// the desired value, which is mapped to a particular value for Brave.
#define ShapeContextTokens ShapeContextTokensOverride

#include <chrome/browser/ui/views/omnibox/rounded_omnibox_results_frame.cc>

#undef ShapeContextTokens

// static
int RoundedOmniboxResultsFrame::GetShadowElevation() {
  // Expose a constant defined in cc file.
  return kElevation;
}
