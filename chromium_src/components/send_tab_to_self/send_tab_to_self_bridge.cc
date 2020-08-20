// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// This patch is to force update the device list, need this to to be aware of
// the situation when one of devices in chain has changed it's
// property, which manages whether the device should be displayed in the list
// where to send the link to
#define BRAVE_SHOULD_UPDATE_TARGET_DEVICE_INFO_LIST return true

#include "../../../../components/send_tab_to_self/send_tab_to_self_bridge.cc"
