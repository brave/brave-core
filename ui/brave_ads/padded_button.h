// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UI_BRAVE_ADS_PADDED_BUTTON_H_
#define BRAVE_UI_BRAVE_ADS_PADDED_BUTTON_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_ripple.h"
#include "ui/views/controls/button/image_button.h"

namespace brave_ads {

// PaddedButtons are ImageButtons whose image can be padded within the button.
// This allows the creation of buttons like the notification close and expand
// buttons whose clickable areas extends beyond their image areas
// (<http://crbug.com/168822>) without the need to create and maintain
// corresponding resource images with alpha padding. In the future, this class
// will also allow for buttons whose touch areas extend beyond their clickable
// area (<http://crbug.com/168856>).
class PaddedButton : public views::ImageButton {
 public:
  explicit PaddedButton(PressedCallback callback);
  ~PaddedButton() override = default;

  std::unique_ptr<views::InkDrop> CreateInkDrop() override;
  void OnThemeChanged() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PaddedButton);
};

}  // namespace brave_ads

#endif  // BRAVE_UI_BRAVE_ADS_PADDED_BUTTON_H_
