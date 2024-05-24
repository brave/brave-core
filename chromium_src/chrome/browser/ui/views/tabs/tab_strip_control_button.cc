/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip_control_button.h"

#include "chrome/browser/ui/views/chrome_layout_provider.h"

#define TabStripControlButton TabStripControlButton_ChromiumImpl

#include "src/chrome/browser/ui/views/tabs/tab_strip_control_button.cc"

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
