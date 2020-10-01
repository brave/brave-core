/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/mac/foundation_util.h"
#define BaseBundleID BaseBundleID_ChromiumImpl
#include "../../../../base/mac/foundation_util.mm"
#undef BaseBundleID
#include "components/version_info/channel.h"

#if defined(OFFICIAL_BUILD)
// This is layer violation because foundation_util is built with base lib.
// So, only non-component build(official build in our case) should include this
// header. Otherwise, undefined reference link error will be happened.
//
// I think below code will be fine.
//
//  #if defined(OFFICIAL_BUILD)
//    return "com.brave.Browser";
//  #else
//    return "com.brave.Browser.development";
//  #endif
//
// because |base_bundle_id| is set before calling BaseBundleID() in non test
// build and chrome::GetChannel() will always return stable in test build.
//
// For safe in non-test build, current seems fine in official build.
#if defined(OS_IOS)
#include "ios/chrome/common/channel_info.h"
#else
#include "chrome/common/channel_info.h"
#endif
#endif

namespace base {
namespace mac {

const char* BaseBundleID() {
  if (base_bundle_id) {
    return base_bundle_id;
  }

#if !defined(OFFICIAL_BUILD)
  #if defined(OS_IOS)
  return "com.brave.ios.BrowserBeta";
  #else
  return "com.brave.Browser.development";
  #endif
#elif defined(OS_IOS)
  switch (GetChannel()) {
    case version_info::Channel::CANARY:
      return "com.brave.ios.enterprise.Browser";
    case version_info::Channel::DEV:
      return "com.brave.ios.BrowserBeta";
    case version_info::Channel::BETA:
      return "com.brave.ios.beta";
    case version_info::Channel::STABLE:
      return "com.brave.ios.browser";
    case version_info::Channel::UNKNOWN:
    default:
      return "com.brave.Browser";
  }
#else
  switch (chrome::GetChannel()) {
    case version_info::Channel::CANARY:
      return "com.brave.Browser.nightly";
    case version_info::Channel::DEV:
      return "com.brave.Browser.dev";
    case version_info::Channel::BETA:
      return "com.brave.Browser.beta";
    case version_info::Channel::STABLE:
    case version_info::Channel::UNKNOWN:
    default:
      return "com.brave.Browser";
  }
#endif
}

}  // namespace mac
}  // namespace base
