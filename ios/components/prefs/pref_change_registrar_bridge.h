// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_COMPONENTS_PREFS_PREF_CHANGE_REGISTRAR_BRIDGE_H_
#define BRAVE_IOS_COMPONENTS_PREFS_PREF_CHANGE_REGISTRAR_BRIDGE_H_

#import <Foundation/Foundation.h>

@protocol PrefServiceBridge;

NS_ASSUME_NONNULL_BEGIN

@protocol PrefChangeRegistrarProtocol

/// Initialize with a PrefService. Must be called before adding observers
- (void)initializeWithPrefService:(id<PrefServiceBridge>)prefService
    NS_SWIFT_NAME(initialize(with:));

/// Removes all observers and clears the reference to PrefService
/// initializeWithPrefService must be called before adding observers again
- (void)reset;

/// Adds a pref observer for the specified pref path
/// Only one observer may be registered per path
- (void)addObserverForPath:(NSString*)path
                  callback:(void (^)(NSString* prefPath))callback
    NS_SWIFT_NAME(addObserver(forPath:callback:));

/// Removes the pref observer registered for the path
- (void)removeObserverForPath:(NSString*)path
    NS_SWIFT_NAME(removeObserver(forPath:));

/// Removes all observers that have been previously added
- (void)removeAllObservers;

/// Returns true if no pref observers are registered
- (BOOL)isEmpty;

/// Check whether pref path is in the set of preferences being observed
- (BOOL)isObservedPath:(NSString*)path NS_SWIFT_NAME(isObserved(path:));

@end

NS_SWIFT_NAME(PrefChangeRegistrar)
OBJC_EXPORT
@interface PrefChangeRegistrarBridge : NSObject <PrefChangeRegistrarProtocol>
/// Creates a PrefChangeRegistrarBridge and automatically initializes it with
/// the given pref service.
- (instancetype)initWithPrefService:(id<PrefServiceBridge>)prefService;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_COMPONENTS_PREFS_PREF_CHANGE_REGISTRAR_BRIDGE_H_
