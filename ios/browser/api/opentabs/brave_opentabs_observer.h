/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_OBSERVER_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_OBSERVER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@protocol OpenTabsSessionStateObserver <NSObject>

/// Observing open tabs synced session data changes.
/// Notifying when `synced session` info should be updated.
- (void)openTabsSyncStateChanged;

@optional

/// Handling sync cycle completed events in a special way
- (void)openTabsSyncCycleCompleted;

/// Observing synced session when sync cycle is completed
- (void)openTabsSyncConfigurationCompleted;

/// Called when the sync service is being shutdown permanently
- (void)openTabsSyncShutdown;

@end

OBJC_EXPORT
@protocol OpenTabsSessionStateListener
- (void)destroy;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_OPENTABS_OBSERVER_H_
