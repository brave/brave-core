/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/channel_info.h"

#include "components/version_info/version_info.h"

#define GetChannelName GetChannelName_ChromiumImpl
#include <chrome/common/channel_info_android.cc>
#undef GetChannelName

namespace chrome {

// On Android, the CANARY channel is branded as Nightly, not Canary.
std::string GetChannelName(WithExtendedStable with_extended_stable) {
  if (GetChannel() == version_info::Channel::CANARY) {
    return "nightly";
  }
  return GetChannelName_ChromiumImpl(with_extended_stable);
}

}  // namespace chrome
