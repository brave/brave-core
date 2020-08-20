// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// This patch allows bubble controller to re-read all devices list each time
// when the menu is shown.
#define BRAVE_GET_VALID_DEVICES \
  (const_cast<SendTabToSelfBubbleController*>(this))->FetchDeviceInfo()

#include "../../../../../../../chrome/browser/ui/send_tab_to_self/send_tab_to_self_bubble_controller.cc"
