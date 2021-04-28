/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ADS_PADDED_IMAGE_VIEW_H_
#define BRAVE_BROWSER_UI_BRAVE_ADS_PADDED_IMAGE_VIEW_H_

#include "ui/views/controls/image_view.h"
#include "ui/views/metadata/metadata_header_macros.h"

namespace brave_ads {

class PaddedImageView : public views::ImageView {
 public:
  METADATA_HEADER(PaddedImageView);

  PaddedImageView();
  ~PaddedImageView() override = default;

 private:
  PaddedImageView(const PaddedImageView&) = delete;
  PaddedImageView& operator=(const PaddedImageView&) = delete;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_BRAVE_ADS_PADDED_IMAGE_VIEW_H_
