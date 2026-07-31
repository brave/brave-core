/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip_control_button.h"

#include "chrome/browser/ui/views/chrome_layout_provider.h"

#define TabStripControlButton TabStripControlButton_ChromiumImpl

// Use the CR foreground token for the default active icon color so ultra-dark
// postprocessing of kColorNewTabButtonCRForegroundFrameActive applies to the
// horizontal new-tab, workspaces, and scroll buttons. Outside darker theme
// this CR id still aliases to kColorTabForegroundInactiveFrameActive.
#define kColorTabForegroundInactiveFrameActive \
  kColorNewTabButtonCRForegroundFrameActive

// Use the CR background tokens (transparent in Brave via
// AddBravifiedTabStripColorMixer) so idle tab-strip control buttons —
// workspaces and scroll chevrons — do not paint a solid frame-colored chip.
// Matches TabStripComboButton / TabStripFlatEdgeButton.
#define kColorNewTabButtonBackgroundFrameActive \
  kColorNewTabButtonCRBackgroundFrameActive
#define kColorNewTabButtonBackgroundFrameInactive \
  kColorNewTabButtonCRBackgroundFrameInactive

#include <chrome/browser/ui/views/tabs/tab_strip_control_button.cc>

#undef kColorNewTabButtonBackgroundFrameInactive
#undef kColorNewTabButtonBackgroundFrameActive
#undef kColorTabForegroundInactiveFrameActive
#undef TabStripControlButton

TabStripControlButton::~TabStripControlButton() = default;

int TabStripControlButton::GetCornerRadius() const {
  // Ensure that tabstrip buttons have the correct rounded rect shape, and not
  // a circular shape.
  return ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, GetContentsBounds().size());
}

BEGIN_METADATA(TabStripControlButton)
END_METADATA
