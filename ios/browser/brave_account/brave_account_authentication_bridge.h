/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_AUTHENTICATION_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_AUTHENTICATION_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_SWIFT_NAME(BraveAccountAuthentication)
@protocol BraveAccountAuthenticationBridge

- (void)resendConfirmationEmail;
- (void)cancelRegistration;
- (void)logOut;

@end

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_AUTHENTICATION_BRIDGE_H_
