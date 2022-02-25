/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// CreateManageDevicesLink requires Google account, which causes a crash so we
// need to early return before getting CreateManageDevicesLink() called.
#define BRAVE_SENT_TAB_TO_SELF_BUBBLE_VIEW_IMPL \
  if (true)                                     \
    return;

#include "src/chrome/browser/ui/views/send_tab_to_self/send_tab_to_self_bubble_view_impl.cc"

#undef BRAVE_SENT_TAB_TO_SELF_BUBBLE_VIEW_IMPL