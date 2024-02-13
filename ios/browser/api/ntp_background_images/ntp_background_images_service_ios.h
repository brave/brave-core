/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_BACKGROUND_IMAGES_SERVICE_IOS_H_
#define BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_BACKGROUND_IMAGES_SERVICE_IOS_H_

#import <Foundation/Foundation.h>

@class NTPBackgroundImage, NTPSponsoredImageData;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface NTPBackgroundImagesService : NSObject

@property(nonatomic, copy, nullable) void (^backgroundImagesUpdated)();
@property(nonatomic, copy, nullable) void (^sponsoredImageDataUpdated)
    (NTPSponsoredImageData* _Nullable data);

@property(readonly) NSArray<NTPBackgroundImage*>* backgroundImages;
@property(readonly, nullable) NTPSponsoredImageData* sponsoredImageData;
@property(readonly, nullable) NTPSponsoredImageData* superReferralImageData;

// TODO(https://github.com/brave/brave-core/pull/21559): Remove these properties
// once we have a better way to handle Griffin feature params from iOS.
@property(nonatomic, readonly) NSInteger initialCountToBrandedWallpaper;
@property(nonatomic, readonly) NSInteger countToBrandedWallpaper;

- (void)updateSponsoredImageComponentIfNeeded;

@property(readonly) NSString* superReferralCode;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_BACKGROUND_IMAGES_SERVICE_IOS_H_
