/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/infobars/core/infobar.cc"

namespace infobars {

void InfoBar::BraveSetTargetHeight(int height) {
  target_height_ = height;
  height_ = animation_.CurrentValueBetween(0, target_height_);
}

}  // namespace infobars
