/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// CreateManageDevicesLink requires Google account, which causes a crash so we
// need to early return before getting CreateManageDevicesLink() called.
#define BRAVE_SEND_TAB_TO_SELFDEVICE_PICKER_BUBBLE_VIEW_INIT \
  if (true)                                                  \
    return;

#include "src/chrome/browser/ui/views/send_tab_to_self/send_tab_to_self_device_picker_bubble_view.cc"

#undef BRAVE_SEND_TAB_TO_SELFDEVICE_PICKER_BUBBLE_VIEW_INIT
