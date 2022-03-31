/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/version_info/version_info_ios.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/version_info/version_info.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BraveCoreVersionInfo

+ (NSString*)braveCoreVersion {
  return base::SysUTF8ToNSString(
      version_info::GetBraveVersionWithoutChromiumMajorVersion());
}

+ (NSString*)chromiumVersion {
  return base::SysUTF8ToNSString(version_info::GetBraveChromiumVersionNumber());
}

@end
