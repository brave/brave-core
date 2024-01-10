/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_VERSION_INFO_CHANNEL_H_
#define BRAVE_CHROMIUM_SRC_BASE_VERSION_INFO_CHANNEL_H_

#include <string_view>

#define GetChannelString GetChannelString_ChromiumImpl

#include "src/base/version_info/channel.h"  // IWYU pragma: export
#undef GetChannelString

namespace version_info {

// We use |nightly| instead of |canary|.
constexpr std::string_view GetChannelString(Channel channel) {
  if (channel == Channel::CANARY) {
    return "nightly";
  }

  return GetChannelString_ChromiumImpl(channel);
}

}  // namespace version_info

#endif  // BRAVE_CHROMIUM_SRC_BASE_VERSION_INFO_CHANNEL_H_
