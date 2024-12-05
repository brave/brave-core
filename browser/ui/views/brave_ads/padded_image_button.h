/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_PADDED_IMAGE_BUTTON_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_PADDED_IMAGE_BUTTON_H_

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/button/image_button.h"

namespace views {
class InkDrop;
}  // namespace views

namespace brave_ads {

// PaddedImageButtons are ImageButtons whose image can be padded within the
// button. This allows the creation of buttons whose clickable areas extend
// beyond their image areas without the need to create and maintain
// corresponding resource images with alpha padding
class PaddedImageButton : public views::ImageButton {
  METADATA_HEADER(PaddedImageButton, views::ImageButton)
 public:

  explicit PaddedImageButton(PressedCallback callback);

  PaddedImageButton(const PaddedImageButton&) = delete;
  PaddedImageButton& operator=(const PaddedImageButton&) = delete;

  ~PaddedImageButton() override = default;

  void AdjustBorderInsetToFitHeight(const int height);

  // views::Button:
  void OnThemeChanged() override;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_PADDED_IMAGE_BUTTON_H_
