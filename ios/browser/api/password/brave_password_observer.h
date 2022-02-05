/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_PASSWORD_BRAVE_PASSWORD_OBSERVER_H_
#define BRAVE_IOS_BROWSER_API_PASSWORD_BRAVE_PASSWORD_OBSERVER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class IOSPasswordForm;

OBJC_EXPORT
@protocol PasswordStoreObserver <NSObject>
@optional

/// Observing password data changes, does not contain removed entries
/// @param changedFormList - List of credential forms that are updated/changed
- (void)passwordFormsChanged:(NSArray<IOSPasswordForm*>*)changedFormList;

/// Observing password data changes
/// @param retainedFormList - Complete list of passwords and blocklisted sites
- (void)passwordFormsRetained:(NSArray<IOSPasswordForm*>*)retainedFormList;

@end

OBJC_EXPORT
@protocol PasswordStoreListener
- (void)destroy;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_PASSWORD_BRAVE_PASSWORD_OBSERVER_H_
