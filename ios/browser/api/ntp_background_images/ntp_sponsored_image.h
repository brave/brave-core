/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_IMAGE_H_
#define BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_IMAGE_H_

#import <Foundation/Foundation.h>

@class NTPSponsoredImageCampaign, NTPSponsoredImageBackground,
    NTPSponsoredImageLogo, NTPSponsoredImageTopSite;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface NTPSponsoredImageData : NSObject
@property(readonly) NSArray<NTPSponsoredImageCampaign*>* campaigns;
@property(readonly) BOOL isSuperReferral;
@property(readonly, nullable) NSString* themeName;
@property(readonly, nullable) NSArray<NTPSponsoredImageTopSite*>* topSites;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)
    initWithCampaigns:(NSArray<NTPSponsoredImageCampaign*>*)campaigns
      isSuperReferral:(BOOL)isSuperReferral
            themeName:(nullable NSString*)themeName
             topSites:(nullable NSArray<NTPSponsoredImageTopSite*>*)topSites
    NS_DESIGNATED_INITIALIZER;
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
@property(readonly) CGPoint focalPoint;
@property(readonly) NSString* backgroundColor;
@property(readonly) NSString* creativeInstanceId;
@property(readonly) NTPSponsoredImageLogo* logo;
@property(readonly) CGRect viewBox;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithImagePath:(NSURL*)imagePath
                       focalPoint:(CGPoint)focalPoint
                  backgroundColor:(NSString*)backgroundColor
               creativeInstanceId:(NSString*)creativeInstanceId
                             logo:(NTPSponsoredImageLogo*)logo
                          viewBox:(CGRect)viewBox NS_DESIGNATED_INITIALIZER;
@end

OBJC_EXPORT
@interface NTPSponsoredImageLogo : NSObject
@property(readonly) NSURL* imagePath;
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

OBJC_EXPORT
@interface NTPSponsoredImageTopSite : NSObject
@property(readonly) NSString* name;
@property(readonly, nullable) NSURL* destinationURL;
@property(readonly) NSString* backgroundColor;
@property(readonly) NSURL* imagePath;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithName:(NSString*)name
              destinationURL:(NSURL*)destinationURL
             backgroundColor:(NSString*)backgroundColor
                   imagePath:(NSURL*)imagePath NS_DESIGNATED_INITIALIZER;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_NTP_BACKGROUND_IMAGES_NTP_SPONSORED_IMAGE_H_
