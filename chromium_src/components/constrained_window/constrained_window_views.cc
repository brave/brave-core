/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/web_modal/modal_dialog_host.h"

#define GetDialogPosition(size)                                              \
  GetDialogPosition(size);                                                   \
  if (auto* widget_delegate = widget->widget_delegate();                     \
      widget_delegate && widget_delegate->has_desired_position_delegate()) { \
    position = widget_delegate->get_desired_position();                      \
  }

#include "src/components/constrained_window/constrained_window_views.cc"

#undef GetDialogPosition
