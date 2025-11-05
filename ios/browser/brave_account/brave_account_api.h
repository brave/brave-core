/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_API_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_API_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT NSString* const BraveAccountAuthenticationTokenPref;
OBJC_EXPORT NSString* const BraveAccountVerificationTokenPref;

OBJC_EXPORT
@interface BraveAccountAPI : NSObject
- (instancetype)init NS_UNAVAILABLE;
- (void)resendConfirmationEmail;
- (void)cancelRegistration;
- (void)logOut;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_API_H_
