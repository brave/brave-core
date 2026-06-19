// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_COSMETIC_FILTERING_COSMETIC_FILTERING_ARGS_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_COSMETIC_FILTERING_COSMETIC_FILTERING_ARGS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface CosmeticFilteringArgs : NSObject <NSCopying, NSSecureCoding>

@property(nonatomic, assign) BOOL hideFirstPartyContent;
@property(nonatomic, assign) BOOL genericHide;
@property(nonatomic, strong, nullable) NSNumber* firstSelectorsPollingDelayMs;
@property(nonatomic, strong, nullable)
    NSNumber* switchToSelectorsPollingThreshold;
@property(nonatomic, strong, nullable)
    NSNumber* fetchNewClassIdRulesThrottlingMs;
@property(nonatomic, strong, nonnull) NSSet<NSString*>* aggressiveSelectors;
@property(nonatomic, strong, nonnull) NSSet<NSString*>* standardSelectors;
@property(nonatomic, strong, nonnull) NSSet<NSString*>* proceduralFilters;

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

- (instancetype)initWithHideFirstPartyContent:(BOOL)hideFirstPartyContent
                                  genericHide:(BOOL)genericHide
                 firstSelectorsPollingDelayMs:
                     (nullable NSNumber*)firstSelectorsPollingDelayMs
            switchToSelectorsPollingThreshold:
                (nullable NSNumber*)switchToSelectorsPollingThreshold
             fetchNewClassIdRulesThrottlingMs:
                 (nullable NSNumber*)fetchNewClassIdRulesThrottlingMs
                          aggressiveSelectors:
                              (nonnull NSSet<NSString*>*)aggressiveSelectors
                            standardSelectors:
                                (nonnull NSSet<NSString*>*)standardSelectors
                            proceduralFilters:
                                (nonnull NSSet<NSString*>*)proceduralFilters;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_COSMETIC_FILTERING_COSMETIC_FILTERING_ARGS_H_
