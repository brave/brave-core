// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_COMPONENTS_PREFS_PREF_SERVICE_BRIDGE_H_
#define BRAVE_IOS_COMPONENTS_PREFS_PREF_SERVICE_BRIDGE_H_

#import <Foundation/Foundation.h>

@class BaseValueBridge;

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(PrefService)
@protocol PrefServiceBridge

/// Lands pending writes to disk. This should only be used if we need to save
/// immediately (basically, during shutdown)
- (void)commitPendingWrite;

/// Returns true if the preference for the given preference name is available
/// and is managed.
- (BOOL)isManagedPreferenceForPath:(NSString*)path
    NS_SWIFT_NAME(isManagedPreference(forPath:));

- (BOOL)boolForPath:(NSString*)path NS_SWIFT_NAME(boolean(forPath:));
- (NSInteger)integerForPath:(NSString*)path NS_SWIFT_NAME(integer(forPath:));
- (double)doubleForPath:(NSString*)path NS_SWIFT_NAME(double(forPath:));
- (NSString*)stringForPath:(NSString*)path NS_SWIFT_NAME(string(forPath:));
- (NSString*)filePathForPath:(NSString*)path NS_SWIFT_NAME(filePath(forPath:));
- (BaseValueBridge*)valueForPath:(NSString*)path NS_SWIFT_NAME(value(forPath:));
- (NSDictionary<NSString*, BaseValueBridge*>*)dictionaryForPath:(NSString*)path
    NS_SWIFT_NAME(dictionary(forPath:));
- (NSArray<BaseValueBridge*>*)listForPath:(NSString*)path
    NS_SWIFT_NAME(list(forPath:));

- (void)setBool:(BOOL)value
        forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));
- (void)setInteger:(NSInteger)value
           forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));
- (void)setDouble:(double)value
          forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));
- (void)setString:(NSString*)value
          forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));
- (void)setFilePath:(NSString*)value
            forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));
- (void)setValue:(BaseValueBridge*)value
         forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));
- (void)setDictionary:(NSDictionary<NSString*, BaseValueBridge*>*)dict
              forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));
- (void)setList:(NSArray<BaseValueBridge*>*)list
        forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));

// Int64 helper methods that actually store the given value as a string.
// Note that if obtaining the named value via GetDictionary or GetList, the
// Value type will be Type::STRING.
- (int64_t)int64ForPath:(NSString*)path NS_SWIFT_NAME(int64(forPath:));
- (void)setInt64:(int64_t)value
         forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));
- (uint64_t)uint64ForPath:(NSString*)path NS_SWIFT_NAME(uint64(forPath:));
- (void)setUint64:(uint64_t)value
          forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));

- (NSDate*)timeForPath:(NSString*)path NS_SWIFT_NAME(time(forPath:));
- (void)setTime:(NSDate*)time
        forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));
- (NSTimeInterval)timeDeltaForPath:(NSString*)path
    NS_SWIFT_NAME(timeDelta(forPath:));
- (void)setTimeDelta:(NSTimeInterval)delta
             forPath:(NSString*)path NS_SWIFT_NAME(set(_:forPath:));

/// Removes a user pref and restores the pref to its default value
- (void)clearPrefForPath:(NSString*)path NS_SWIFT_NAME(clearPref(forPath:));

/// Returns the value of the given preference, from the user pref store. If
/// the preference is not set in the user pref store, returns NULL.
- (nullable BaseValueBridge*)userPrefValueForPath:(NSString*)path
    NS_SWIFT_NAME(userPrefValue(forPath:));

/// Checks whether or not a value has been set at a specificed path
- (BOOL)hasPrefPath:(NSString*)path NS_SWIFT_NAME(hasPref(forPath:));

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_COMPONENTS_PREFS_PREF_SERVICE_BRIDGE_H_
