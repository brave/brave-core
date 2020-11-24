// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UI_BRAVE_ADS_PADDED_IMAGE_H_
#define BRAVE_UI_BRAVE_ADS_PADDED_IMAGE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/views/controls/image_view.h"

namespace brave_ads {

class PaddedImage : public views::ImageView {
 public:
  PaddedImage();
  ~PaddedImage() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(PaddedImage);
};

}  // namespace brave_ads

#endif  // BRAVE_UI_BRAVE_ADS_PADDED_IMAGE_H_
