/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define GetChannelString GetChannelString_ChromiumImpl
#define GetProductNameAndVersionForUserAgent GetProductNameAndVersionForUserAgent_Unused
#include "../../../../components/version_info/version_info.cc"
#undef GetChannelString
#undef GetProductNameAndVersionForUserAgent

namespace version_info {

std::string GetProductNameAndVersionForUserAgent() {
  return std::string("Chrome/") + BRAVE_CHROMIUM_VERSION;
}

// We use |nightly| instead of |canary|.
std::string GetChannelString(Channel channel) {
  if (channel == Channel::CANARY)
    return "nightly";

  return GetChannelString_ChromiumImpl(channel);
}

}  // namespace version_info
