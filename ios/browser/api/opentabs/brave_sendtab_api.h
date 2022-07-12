/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_SENDTAB_API_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_SENDTAB_API_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// @protocol SendTabSessionStateObserver;
// @protocol SendTabSessionStateListener;

NS_SWIFT_NAME(BraveSendTabAPI)
OBJC_EXPORT
@interface BraveSendTabAPI : NSObject

// - (id<SendTabSessionStateListener>)addObserver:(id<SendTabSessionStateObserver>)observer;
// - (void)removeObserver:(id<SendTabSessionStateListener>)observer;

- (instancetype)init NS_UNAVAILABLE;

- (void)getListOfSyncedDevices;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_SENDTAB_API_H_
