// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_PROFILE_PROFILE_BRIDGE_H_
#define BRAVE_IOS_BROWSER_API_PROFILE_PROFILE_BRIDGE_H_

#import <Foundation/Foundation.h>

/// A bridge to a Chromium Profile
NS_SWIFT_NAME(Profile)
@protocol ProfileBridge
/// The profiles name, empty if `isOffTheRecord` is true
@property(readonly) NSString* name;
/// Whether or not this profile is a incongnito/private profile
@property(readonly) BOOL isOffTheRecord;
/// Returns the original profile (returns itself when the profile if not OTR)
@property(readonly) id<ProfileBridge> originalProfile;
/// Returns the OTR version of this profile, creates one if it doesn't already
/// exist.
@property(readonly) id<ProfileBridge> offTheRecordProfile;
@end

#endif  // BRAVE_IOS_BROWSER_API_PROFILE_PROFILE_BRIDGE_H_
