/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_SENDTAB_OBSERVER_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_SENDTAB_OBSERVER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class IOSOpenDistantTab;

OBJC_EXPORT
@protocol SendTabToSelfModelStateObserver <NSObject>

/// Invoked when elements of the model are added
- (void)sendTabToSelfEntriesAddedRemotely:
    (NSArray<IOSOpenDistantTab*>*)newEntries;

@optional

/// Invoked when the model has finished loading
- (void)sendTabToSelfModelLoaded;

/// Invoked when elements of the model are removed, or updated
- (void)sendTabToSelfEntriesRemovedRemotely;

/// Notify listeners of new and existing entries
/// that have been marked as opened
- (void)sendTabToSelfEntriesOpenedRemotely;

@end

OBJC_EXPORT
@protocol SendTabToSelfModelStateListener
- (void)destroy;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_SENDTAB_OBSERVER_H_
