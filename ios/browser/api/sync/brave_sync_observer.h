/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_OBSERVER_H_
#define BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_OBSERVER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@protocol SyncPreferenceObserver <NSObject>
@optional

/// Observing if sync account deletion notice is pending
/// @param isPending - Boolean that notifies a pending account deleted notice
- (void)syncAccountDeletedNotice:(bool)isPending;

/// Observing if sync account failed decrypt seed notice is dismissed
/// @param isDismissed - Boolean that notifies a pending account deleted notice
- (void)syncDecryptSeedNotice:(bool)isDismissed;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_OBSERVER_H_
