/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/chrome/common/channel_info.h"

#import <Foundation/Foundation.h>
#import <dispatch/dispatch.h>

#import "base/apple/bundle_locations.h"
#import "base/strings/sys_string_conversions.h"
#import "build/branding_buildflags.h"
#import "components/version_info/version_info.h"
#import "components/version_info/version_string.h"

namespace {

#if defined(OFFICIAL_BUILD)
// Channel of the running application, initialized by the first call to
// GetChannel() and cached for the whole application lifetime.
version_info::Channel g_channel = version_info::Channel::UNKNOWN;
#endif

}  // namespace

std::string GetVersionString() {
  return version_info::GetVersionStringWithModifier(GetChannelString());
}

std::string GetChannelString() {
#if defined(OFFICIAL_BUILD)
  // Only ever return one of "" (for STABLE channel), "unknown", "beta", "dev"
  // or "canary" in branded build.
  switch (GetChannel()) {
    case version_info::Channel::STABLE:
      return std::string();

    case version_info::Channel::BETA:
      return "beta";

    case version_info::Channel::DEV:
      return "dev";

    case version_info::Channel::CANARY:
      return "nightly";

    case version_info::Channel::UNKNOWN:
      return "unknown";
  }
#else
  // Always return empty string for non-branded builds.
  return std::string();
#endif
}

version_info::Channel GetChannel() {
#if defined(OFFICIAL_BUILD)
  static dispatch_once_t channel_dispatch_token;
  dispatch_once(&channel_dispatch_token, ^{
    NSBundle* bundle = base::apple::OuterBundle();

    NSString* channel = [bundle objectForInfoDictionaryKey:@"KSChannelID"];
    if (!channel) {
      // KSChannelID is unset for the stable channel.
      g_channel = version_info::Channel::STABLE;
    } else if ([channel isEqualToString:@"beta"]) {
      g_channel = version_info::Channel::BETA;
    } else if ([channel isEqualToString:@"dev"]) {
      g_channel = version_info::Channel::DEV;
    } else if ([channel isEqualToString:@"nightly"]) {
      g_channel = version_info::Channel::CANARY;
    }
  });

  return g_channel;
#else
  return version_info::Channel::UNKNOWN;
#endif
}
