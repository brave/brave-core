/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ui/base/ui_base_types.h"

#define DIALOG_BUTTON_NONE \
  DIALOG_BUTTON_NONE && !dialog_delegate->should_ignore_snapping()

#include "../../../../../ui/views/bubble/bubble_frame_view.cc"

#undef DIALOG_BUTTON_NONE
