/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define GetChannelString GetChannelString_ChromiumImpl
#define GetProductNameAndVersionForUserAgent \
  GetProductNameAndVersionForUserAgent_Unused

#include "../../../../components/version_info/version_info.cc"
#undef GetChannelString
#undef GetProductNameAndVersionForUserAgent

namespace version_info {

const std::string& GetProductNameAndVersionForUserAgent() {
  static const base::NoDestructor<std::string> product_and_version(
      "Chrome/" + std::string(BRAVE_CHROMIUM_VERSION));
  return *product_and_version;
}

// We use |nightly| instead of |canary|.
std::string GetChannelString(Channel channel) {
  if (channel == Channel::CANARY)
    return "nightly";

  return GetChannelString_ChromiumImpl(channel);
}

}  // namespace version_info
