// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_WINDOW_DIALOG_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_WINDOW_DIALOG_DELEGATE_H_

#define ResetViewShownTimeStampForTesting                                 \
  set_should_ignore_snapping(bool should_ignore_snapping) {               \
    should_ignore_snapping_ = should_ignore_snapping;                     \
  }                                                                       \
  bool should_ignore_snapping() const { return should_ignore_snapping_; } \
                                                                          \
 private:                                                                 \
  bool should_ignore_snapping_ = false;                                   \
                                                                          \
 public:                                                                  \
  void ResetViewShownTimeStampForTesting

#include "../../../../../ui/views/window/dialog_delegate.h"

#undef ResetViewShownTimeStampForTesting

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_WINDOW_DIALOG_DELEGATE_H_
