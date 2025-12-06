/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/ntp_background_images/ntp_sponsored_image.h"

#include "base/files/file_path.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/ios/browser/api/ntp_background_images/ntp_sponsored_image+private.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface NTPSponsoredImageData ()
@property(nonatomic, copy) NSArray<NTPSponsoredImageCampaign*>* campaigns;
@end

@implementation NTPSponsoredImageData

- (instancetype)initWithCampaigns:
    (NSArray<NTPSponsoredImageCampaign*>*)campaigns {
  if ((self = [super init])) {
    self.campaigns = campaigns;
  }
  return self;
}

- (instancetype)initWithData:
    (const ntp_background_images::NTPSponsoredImagesData&)data {
  auto campaigns = [[NSMutableArray<NTPSponsoredImageCampaign*> alloc] init];
  for (const auto& campaign : data.campaigns) {
    [campaigns addObject:[[NTPSponsoredImageCampaign alloc]
                             initWithCampaign:campaign]];
  }

  return [self initWithCampaigns:campaigns];
}

@end

@interface NTPSponsoredImageCampaign ()
@property(nonatomic, copy) NSString* campaignId;
@property(nonatomic, copy) NSArray<NTPSponsoredImageBackground*>* backgrounds;
@end

@implementation NTPSponsoredImageCampaign

- (instancetype)initWithCampaignId:(NSString*)campaignId
                       backgrounds:
                           (NSArray<NTPSponsoredImageBackground*>*)backgrounds {
  if ((self = [super init])) {
    self.campaignId = campaignId;
    self.backgrounds = backgrounds;
  }
  return self;
}

- (instancetype)initWithCampaign:
    (const ntp_background_images::Campaign&)campaign {
  auto campaignId = base::SysUTF8ToNSString(campaign.campaign_id);
  auto backgrounds =
      [[NSMutableArray<NTPSponsoredImageBackground*> alloc] init];
  for (const auto& creative : campaign.creatives) {
    [backgrounds addObject:[[NTPSponsoredImageBackground alloc]
                               initWithSponsoredBackground:creative]];
  }
  return [self initWithCampaignId:campaignId backgrounds:backgrounds];
}

@end

@interface NTPSponsoredImageBackground ()
@property(nonatomic, copy) NSURL* imagePath;
@property(nonatomic) BOOL isRichMedia;
@property(nonatomic) CGPoint focalPoint;
@property(nonatomic, copy) NSString* creativeInstanceId;
@property(nonatomic) NTPSponsoredImageLogo* logo;
@property(nonatomic) BraveAdsNewTabPageAdMetricType metricType;
@end

@implementation NTPSponsoredImageBackground

- (instancetype)initWithImagePath:(NSURL*)imagePath
                      isRichMedia:(BOOL)isRichMedia
                       focalPoint:(CGPoint)focalPoint
               creativeInstanceId:(NSString*)creativeInstanceId
                             logo:(NTPSponsoredImageLogo*)logo
                       metricType:(BraveAdsNewTabPageAdMetricType)metricType {
  if ((self = [super init])) {
    self.imagePath = imagePath;
    self.isRichMedia = isRichMedia;
    self.focalPoint = focalPoint;
    self.creativeInstanceId = creativeInstanceId;
    self.logo = logo;
    self.metricType = metricType;
  }
  return self;
}

- (instancetype)initWithSponsoredBackground:
    (const ntp_background_images::Creative&)sponsoredBackground {
  auto imagePath =
      [NSURL fileURLWithPath:base::SysUTF8ToNSString(
                                 sponsoredBackground.file_path.value())];
  bool isRichMedia = sponsoredBackground.wallpaper_type ==
                     ntp_background_images::WallpaperType::kRichMedia;
  auto focalPoint = sponsoredBackground.focal_point.ToCGPoint();
  auto creativeInstanceId =
      base::SysUTF8ToNSString(sponsoredBackground.creative_instance_id);
  auto logo =
      [[NTPSponsoredImageLogo alloc] initWithLogo:sponsoredBackground.logo];
  auto metricType = sponsoredBackground.metric_type;
  return [self initWithImagePath:imagePath
                     isRichMedia:isRichMedia
                      focalPoint:focalPoint
              creativeInstanceId:creativeInstanceId
                            logo:logo
                      metricType:(BraveAdsNewTabPageAdMetricType)metricType];
}

@end

@interface NTPSponsoredImageLogo ()
@property(nonatomic, copy, nullable) NSURL* imagePath;
@property(nonatomic, copy) NSString* altText;
@property(nonatomic, copy, nullable) NSURL* destinationURL;
@property(nonatomic, copy) NSString* companyName;
@end

@implementation NTPSponsoredImageLogo

- (instancetype)initWithImagePath:(NSURL*)imagePath
                          altText:(NSString*)altText
                   destinationURL:(NSURL*)destinationURL
                      companyName:(NSString*)companyName {
  if ((self = [super init])) {
    self.imagePath = imagePath;
    self.altText = altText;
    self.destinationURL = destinationURL;
    self.companyName = companyName;
  }
  return self;
}

- (instancetype)initWithLogo:(const ntp_background_images::Logo&)logo {
  auto imagePath =
      [NSURL fileURLWithPath:base::SysUTF8ToNSString(logo.image_file.value())];
  auto altText = base::SysUTF8ToNSString(logo.alt_text);
  auto destinationURL =
      [NSURL URLWithString:base::SysUTF8ToNSString(logo.destination_url)];
  auto companyName = base::SysUTF8ToNSString(logo.company_name);
  return [self initWithImagePath:imagePath
                         altText:altText
                  destinationURL:destinationURL
                     companyName:companyName];
}

@end
