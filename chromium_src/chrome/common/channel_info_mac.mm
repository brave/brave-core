/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/channel_info.h"

#import <Foundation/Foundation.h>

#include <tuple>

#include "base/apple/bundle_locations.h"
#include "base/no_destructor.h"
#include "base/strings/sys_string_conversions.h"
#include "components/version_info/version_info.h"

namespace chrome {

std::string GetChannelName(WithExtendedStable with_extended_stable) {
#if defined(OFFICIAL_BUILD)
  static const base::NoDestructor<std::string> channel([] {
    // Use the main Chrome application bundle and not the framework bundle.
    // Keystone keys don't live in the framework.
    NSBundle* bundle = base::apple::OuterBundle();

    NSString* channel = [bundle objectForInfoDictionaryKey:@"KSChannelID"];

    // Only ever return "", "unknown", "beta", "dev", or "nightly" in an
    // official build.
    if (!channel) {
      // For the stable channel, KSChannelID is not set.
      channel = @"";
    } else if ([channel isEqual:@"beta"] || [channel isEqual:@"dev"] ||
               [channel isEqual:@"nightly"]) {
      // do nothing.
    } else {
      channel = @"unknown";
    }
    return base::SysNSStringToUTF8(channel);
  }());
  return *channel;
#else
  return std::string();
#endif
}

void CacheChannelInfo() {
  std::ignore = GetChannelName(chrome::WithExtendedStable(false));
}

version_info::Channel GetChannelByName(const std::string& channel) {
#if defined(OFFICIAL_BUILD)
  if (channel.empty()) {
    return version_info::Channel::STABLE;
  } else if (channel == "beta") {
    return version_info::Channel::BETA;
  } else if (channel == "dev") {
    return version_info::Channel::DEV;
  } else if (channel == "nightly") {
    return version_info::Channel::CANARY;
  }
#endif

  return version_info::Channel::UNKNOWN;
}

version_info::Channel GetChannel() {
  return GetChannelByName(GetChannelName(chrome::WithExtendedStable(false)));
}

bool IsExtendedStableChannel() {
  // No extended stable channel for Brave.
  return false;
}

bool IsSideBySideCapable() {
  return true;
}

}  // namespace chrome
