/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/version_info/version_info_ios.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/version_info/version_info.h"
#include "components/version_info/version_info.h"
#include "components/version_info/version_string.h"
#include "ios/chrome/common/channel_info.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BraveCoreVersionInfoChannel const BraveCoreVersionInfoChannelStable =
    static_cast<NSInteger>(version_info::Channel::STABLE);
BraveCoreVersionInfoChannel const BraveCoreVersionInfoChannelBeta =
    static_cast<NSInteger>(version_info::Channel::BETA);
BraveCoreVersionInfoChannel const BraveCoreVersionInfoChannelDevelopment =
    static_cast<NSInteger>(version_info::Channel::DEV);
BraveCoreVersionInfoChannel const BraveCoreVersionInfoChannelNightly =
    static_cast<NSInteger>(version_info::Channel::CANARY);
BraveCoreVersionInfoChannel const BraveCoreVersionInfoChannelUnknown =
    static_cast<NSInteger>(version_info::Channel::UNKNOWN);

@implementation BraveCoreVersionInfo

+ (NSString*)braveCoreVersion {
  return base::SysUTF8ToNSString(
      version_info::GetBraveVersionWithoutChromiumMajorVersion());
}

+ (NSString*)chromiumVersion {
  return base::SysUTF8ToNSString(version_info::GetBraveChromiumVersionNumber());
}

+ (NSString*)channelString {
  return base::SysUTF8ToNSString(GetChannelString());
}

+ (BraveCoreVersionInfoChannel)channel {
  switch (GetChannel()) {
    case version_info::Channel::STABLE:
      return BraveCoreVersionInfoChannelStable;
    case version_info::Channel::BETA:
      return BraveCoreVersionInfoChannelBeta;
    case version_info::Channel::DEV:
      return BraveCoreVersionInfoChannelDevelopment;
    case version_info::Channel::CANARY:
      return BraveCoreVersionInfoChannelNightly;
    case version_info::Channel::UNKNOWN:
      return BraveCoreVersionInfoChannelUnknown;
  }
}
@end
