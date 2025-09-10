// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ui/base/resource/resource_bundle.h"

#include "base/no_destructor.h"

#define GetThemedLottieImageNamed GetThemedLottieImageNamed_Unused
#include <ui/base/resource/resource_bundle.cc>
#undef GetThemedLottieImageNamed

namespace ui {

const ui::ImageModel& ResourceBundle::GetThemedLottieImageNamed(
    int resource_id) {
  // Block all Lottie assets
  static base::NoDestructor<ui::ImageModel> empty_image;
  return *empty_image;
}

}  // namespace ui
