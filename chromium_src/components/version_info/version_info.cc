/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define GetChannelString GetChannelString_ChromiumImpl
#include "../../../../components/version_info/version_info.cc"
#undef GetChannelString

namespace version_info {

// We use |nightly| instead of |canary|.
std::string GetChannelString(Channel channel) {
  if (channel == Channel::CANARY)
    return "nightly";

  return GetChannelString_ChromiumImpl(channel);
}

}  // namespace version_info
