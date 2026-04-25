/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_TABGENERATOR_API_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_TABGENERATOR_API_H_

#import <Foundation/Foundation.h>

@class WebState;

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(BraveSyncTab)
OBJC_EXPORT
@interface BraveSyncTab : NSObject

@property(nonatomic, strong, readonly) WebState* webState;

- (instancetype)init NS_UNAVAILABLE;

/// Function setting Title for the created Sync Tab
/// @param title Title for the Tab represantation
- (void)setTitle:(NSString*)title;
/// Function setting URL for the created Sync Tab
/// @param url URL for the tab represantation
- (void)setURL:(NSURL*)url;
@end

NS_SWIFT_NAME(BraveTabGeneratorAPI)
OBJC_EXPORT
@interface BraveTabGeneratorAPI : NSObject

- (instancetype)init NS_UNAVAILABLE;

- (BraveSyncTab*)createBraveSyncTab:(bool)isOffTheRecord
    NS_SWIFT_NAME(createBraveSyncTab(isOffTheRecord:));
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_TABGENERATOR_API_H_
