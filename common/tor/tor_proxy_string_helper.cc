/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/tor/tor_proxy_string_helper.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/channel.h"

namespace tor {

std::string GetTorProxyString() {
  switch (chrome::GetChannel()) {
    case version_info::Channel::STABLE:
      return std::string("socks5://127.0.0.1:9350");
    case version_info::Channel::BETA:
      return std::string("socks5://127.0.0.1:9360");
    case version_info::Channel::DEV:
      return std::string("socks5://127.0.0.1:9370");
    case version_info::Channel::CANARY:
      return std::string("socks5://127.0.0.1:9380");
    case version_info::Channel::UNKNOWN:
    default:
      return std::string("socks5://127.0.0.1:9390");
  }
}

}  // namespace tor
