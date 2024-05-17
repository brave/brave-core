/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/deprecated_channel_name/channel_name.h"

namespace deprecated_channel_name {

std::string GetChannelName(version_info::Channel channel) {
#if defined(OFFICIAL_BUILD)
  if (channel == version_info::Channel::STABLE) {
    return "release";
  }

  return std::string(version_info::GetChannelString(channel));
#else   // OFFICIAL_BUILD
  return "developer";
#endif  // !OFFICIAL_BUILD
}

}  // namespace deprecated_channel_name
