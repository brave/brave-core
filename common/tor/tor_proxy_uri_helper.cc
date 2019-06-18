/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/tor/tor_proxy_uri_helper.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/channel.h"

namespace tor {

std::string GetTorProxyURI() {
  const std::string TorProxyScheme("socks5://");
  const std::string TorProxyAddress("127.0.0.1");
  std::string port;
  switch (chrome::GetChannel()) {
    case version_info::Channel::STABLE:
      port = std::string("9350");
      break;
    case version_info::Channel::BETA:
      port = std::string("9360");
      break;
    case version_info::Channel::DEV:
      port = std::string("9370");
      break;
    case version_info::Channel::CANARY:
      port = std::string("9380");
      break;
    case version_info::Channel::UNKNOWN:
    default:
      port = std::string("9390");
  }
  return std::string(TorProxyScheme + TorProxyAddress + ":" + port);
}

}  // namespace tor
