// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_ANDROID_VIEW_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_UI_ANDROID_VIEW_ANDROID_H_

#define OnDragEvent                               \
  Unused() {                                      \
    return false;                                 \
  }                                               \
  friend class speedreader::SpeedreaderTabHelper; \
  bool OnDragEvent

namespace speedreader {
class SpeedreaderTabHelper;
}

#include "src/ui/android/view_android.h"  // IWYU pragma: export

#undef OnDragEvent

#endif  // BRAVE_CHROMIUM_SRC_UI_ANDROID_VIEW_ANDROID_H_
