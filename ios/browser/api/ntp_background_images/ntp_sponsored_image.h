/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_IMAGE_H_
#define BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_IMAGE_H_

#import <Foundation/Foundation.h>

#ifdef __cplusplus
#include "brave/ios/browser/api/ads/brave_ads.mojom.objc.h"
#else
#import "brave_ads.mojom.objc.h"
#endif

@class NTPSponsoredImageCampaign, NTPSponsoredImageBackground,
    NTPSponsoredImageLogo;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface NTPSponsoredImageData : NSObject
@property(readonly) NSArray<NTPSponsoredImageCampaign*>* campaigns;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCampaigns:
    (NSArray<NTPSponsoredImageCampaign*>*)campaigns NS_DESIGNATED_INITIALIZER;
@end

OBJC_EXPORT
@interface NTPSponsoredImageCampaign : NSObject
@property(readonly) NSString* campaignId;
@property(readonly) NSArray<NTPSponsoredImageBackground*>* backgrounds;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCampaignId:(NSString*)campaignId
                       backgrounds:
                           (NSArray<NTPSponsoredImageBackground*>*)backgrounds
    NS_DESIGNATED_INITIALIZER;
@end

OBJC_EXPORT
@interface NTPSponsoredImageBackground : NSObject
@property(readonly) NSURL* imagePath;
@property(readonly) BOOL isRichMedia;
@property(readonly) CGPoint focalPoint;
@property(readonly) NSString* creativeInstanceId;
@property(readonly) NTPSponsoredImageLogo* logo;
@property(readonly) BraveAdsNewTabPageAdMetricType metricType;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithImagePath:(NSURL*)imagePath
                       isRichMedia:(BOOL)isRichMedia
                       focalPoint:(CGPoint)focalPoint
               creativeInstanceId:(NSString*)creativeInstanceId
                             logo:(NTPSponsoredImageLogo*)logo
                       metricType:(BraveAdsNewTabPageAdMetricType)metricType
    NS_DESIGNATED_INITIALIZER;
@end

OBJC_EXPORT
@interface NTPSponsoredImageLogo : NSObject
@property(readonly, nullable) NSURL* imagePath;
@property(readonly) NSString* altText;
@property(readonly, nullable) NSURL* destinationURL;
@property(readonly) NSString* companyName;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithImagePath:(NSURL*)imagePath
                          altText:(NSString*)altText
                   destinationURL:(NSURL*)destinationURL
                      companyName:(NSString*)companyName
    NS_DESIGNATED_INITIALIZER;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_IMAGE_H_
