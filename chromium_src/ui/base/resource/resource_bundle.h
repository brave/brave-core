// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_BASE_RESOURCE_RESOURCE_BUNDLE_H_
#define BRAVE_CHROMIUM_SRC_UI_BASE_RESOURCE_RESOURCE_BUNDLE_H_

#define GetThemedLottieImageNamed             \
  GetThemedLottieImageNamed(int resource_id); \
  const ui::ImageModel& GetThemedLottieImageNamed_Unused
#include <ui/base/resource/resource_bundle.h>  // IWYU pragma: export
#undef GetThemedLottieImageNamed

#endif  // BRAVE_CHROMIUM_SRC_UI_BASE_RESOURCE_RESOURCE_BUNDLE_H_
