// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/profile/profile_bridge_impl.h"

#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

@implementation ProfileBridgeImpl

- (instancetype)initWithProfile:(raw_ptr<ProfileIOS>)profile {
  if ((self = [super init])) {
    _profile = profile;
  }
  return self;
}

- (NSString*)name {
  return base::SysUTF8ToNSString(self.profile->GetProfileName());
}

- (BOOL)isOffTheRecord {
  return self.profile->IsOffTheRecord();
}

- (id<ProfileBridge>)originalProfile {
  return [[ProfileBridgeImpl alloc]
      initWithProfile:self.profile->GetOriginalProfile()];
}

- (id<ProfileBridge>)offTheRecordProfile {
  return [[ProfileBridgeImpl alloc]
      initWithProfile:self.profile->GetOffTheRecordProfile()];
}

@end
