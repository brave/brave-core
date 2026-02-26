// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_USER_AGENT_IOS_COMMON_BRAVE_USER_AGENT_TYPE_H_
#define BRAVE_COMPONENTS_BRAVE_USER_AGENT_IOS_COMMON_BRAVE_USER_AGENT_TYPE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, BraveIOSUserAgentType) {
  /// Safari UA. Used when `kUseBraveUserAgent` flag is disabled.
  BraveIOSUserAgentTypeMasked,
  /// Brave/1
  BraveIOSUserAgentTypeVersion,
  /// Safari UA with Brave suffix
  BraveIOSUserAgentTypeSuffix,
  /// Safari UA with (Brave) suffix
  BraveIOSUserAgentTypeSuffixComment
} NS_SWIFT_NAME(BraveUserAgentType);

OBJC_EXPORT BraveIOSUserAgentType GetDefaultBraveIOSUserAgentType();

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_BRAVE_USER_AGENT_IOS_COMMON_BRAVE_USER_AGENT_TYPE_H_
