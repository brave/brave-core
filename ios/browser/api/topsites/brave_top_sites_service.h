// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(BraveTopSite)
@interface BraveTopSite : NSObject

@property(nonatomic, readonly) NSURL* url;
@property(nonatomic, readonly) NSString* title;

- (instancetype)init NS_UNAVAILABLE;

@end

OBJC_EXPORT
@interface BraveTopSitesService : NSObject

@property(nonatomic, readwrite) NSArray<BraveTopSite*>* tiles;

- (instancetype)init NS_UNAVAILABLE;

- (void)addOrRemoveBlockedUrl:(NSURL*)url add:(BOOL)add;

@end

NS_ASSUME_NONNULL_END
