/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_OMNIBOX_CHIP_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_OMNIBOX_CHIP_BUTTON_H_

// Prevent GetCornerRadius redefinition from multiple (in)direct headers.
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/window/dialog_delegate.h"

#define GetCornerRadius virtual GetCornerRadius
#define OmniboxChipButton OmniboxChipButton_ChromiumImpl

#include "src/chrome/browser/ui/views/location_bar/omnibox_chip_button.h"  // IWYU pragma: export

#undef OmniboxChipButton
#undef GetCornerRadius

class OmniboxChipButton : public OmniboxChipButton_ChromiumImpl {
 public:
  METADATA_HEADER(OmniboxChipButton);
  explicit OmniboxChipButton(PressedCallback callback);

  // OmniboxChipButton_ChromiumImpl overrides:
  int GetCornerRadius() const override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_LOCATION_BAR_OMNIBOX_CHIP_BUTTON_H_
