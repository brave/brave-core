/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/location_bar/omnibox_chip_button.h"

#define OmniboxChipButton OmniboxChipButton_ChromiumImpl
#include "src/chrome/browser/ui/views/location_bar/omnibox_chip_button.cc"
#undef OmniboxChipButton

OmniboxChipButton::OmniboxChipButton(PressedCallback callback)
    : OmniboxChipButton_ChromiumImpl(std::move(callback)) {
  // Overridden method(GetCornerRadius()) is not used in base class' ctor.
  // Set radius again.
  SetCornerRadius(GetCornerRadius());
}

int OmniboxChipButton::GetCornerRadius() const {
  if (const auto* layout_provider = GetLayoutProvider()) {
    return layout_provider->GetCornerRadiusMetric(views::Emphasis::kHigh);
  }
  return 4;
}

BEGIN_METADATA(OmniboxChipButton, OmniboxChipButton_ChromiumImpl)
END_METADATA
