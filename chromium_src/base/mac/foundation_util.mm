/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/mac/foundation_util.h"
#define BaseBundleID BaseBundleID_ChromiumImpl
#include "../../../../base/mac/foundation_util.mm"
#undef BaseBundleID
#include "components/version_info/channel.h"

namespace base {
namespace mac {

const char* BaseBundleID() {
  if (base_bundle_id) {
    return base_bundle_id;
  }

#if !defined(OFFICIAL_BUILD)
  return "org.brave.Brave.development";
#else
  switch (chrome::GetChannel()) {
    case version_info::Channel::CANARY:
      return "org.brave.Brave.nightly";
    case version_info::Channel::DEV:
      return "org.brave.Brave.dev";
    case version_info::Channel::BETA:
      return "org.brave.Brave.beta";
    case version_info::Channel::STABLE:
    case version_info::Channel::UNKNOWN:
    default:
      return "org.brave.Brave";
  }
#endif
}

}  // namespace mac
}  // namespace base
