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
@property(nonatomic) BOOL isSuperReferral;
@property(nonatomic, copy, nullable) NSString* themeName;
@property(nonatomic, copy, nullable)
    NSArray<NTPSponsoredImageTopSite*>* topSites;
@end

@implementation NTPSponsoredImageData

- (instancetype)
    initWithCampaigns:(NSArray<NTPSponsoredImageCampaign*>*)campaigns
      isSuperReferral:(BOOL)isSuperReferral
            themeName:(nullable NSString*)themeName
             topSites:(nullable NSArray<NTPSponsoredImageTopSite*>*)topSites {
  if ((self = [super init])) {
    self.campaigns = campaigns;
    self.isSuperReferral = isSuperReferral;
    self.themeName = themeName;
    self.topSites = topSites;
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
  auto isSuperReferral = data.IsSuperReferral();
  NSString* themeName = nil;
  NSMutableArray<NTPSponsoredImageTopSite*>* topSites = nil;
  if (isSuperReferral) {
    themeName = base::SysUTF8ToNSString(data.theme_name);
    topSites = [[NSMutableArray<NTPSponsoredImageTopSite*> alloc] init];
    for (const auto& topSite : data.top_sites) {
      [topSites
          addObject:[[NTPSponsoredImageTopSite alloc] initWithTopSite:topSite]];
    }
  }
  return [self initWithCampaigns:campaigns
                 isSuperReferral:isSuperReferral
                       themeName:themeName
                        topSites:topSites];
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
  for (const auto& background : campaign.backgrounds) {
    [backgrounds addObject:[[NTPSponsoredImageBackground alloc]
                               initWithSponsoredBackground:background]];
  }
  return [self initWithCampaignId:campaignId backgrounds:backgrounds];
}

@end

@interface NTPSponsoredImageBackground ()
@property(nonatomic, copy) NSURL* imagePath;
@property(nonatomic) CGPoint focalPoint;
@property(nonatomic, copy) NSString* backgroundColor;
@property(nonatomic, copy) NSString* creativeInstanceId;
@property(nonatomic) NTPSponsoredImageLogo* logo;
@property(nonatomic) CGRect viewBox;
@end

@implementation NTPSponsoredImageBackground

- (instancetype)initWithImagePath:(NSURL*)imagePath
                       focalPoint:(CGPoint)focalPoint
                  backgroundColor:(NSString*)backgroundColor
               creativeInstanceId:(NSString*)creativeInstanceId
                             logo:(NTPSponsoredImageLogo*)logo
                          viewBox:(CGRect)viewBox {
  if ((self = [super init])) {
    self.imagePath = imagePath;
    self.focalPoint = focalPoint;
    self.backgroundColor = backgroundColor;
    self.creativeInstanceId = creativeInstanceId;
    self.logo = logo;
    self.viewBox = viewBox;
  }
  return self;
}

- (instancetype)initWithSponsoredBackground:
    (const ntp_background_images::SponsoredBackground&)sponsoredBackground {
  auto imagePath =
      [NSURL fileURLWithPath:base::SysUTF8ToNSString(
                                 sponsoredBackground.image_file.value())];
  auto focalPoint = sponsoredBackground.focal_point.ToCGPoint();
  auto backgroundColor =
      base::SysUTF8ToNSString(sponsoredBackground.background_color);
  auto creativeInstanceId =
      base::SysUTF8ToNSString(sponsoredBackground.creative_instance_id);
  auto logo =
      [[NTPSponsoredImageLogo alloc] initWithLogo:sponsoredBackground.logo];
  auto viewBox = sponsoredBackground.viewbox.value_or(gfx::Rect()).ToCGRect();
  return [self initWithImagePath:imagePath
                      focalPoint:focalPoint
                 backgroundColor:backgroundColor
              creativeInstanceId:creativeInstanceId
                            logo:logo
                         viewBox:viewBox];
}

@end

@interface NTPSponsoredImageLogo ()
@property(nonatomic, copy) NSURL* imagePath;
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

@interface NTPSponsoredImageTopSite ()
@property(nonatomic, copy) NSString* name;
@property(nonatomic, copy, nullable) NSURL* destinationURL;
@property(nonatomic, copy) NSString* backgroundColor;
@property(nonatomic, copy) NSURL* imagePath;
@end

@implementation NTPSponsoredImageTopSite

- (instancetype)initWithName:(NSString*)name
              destinationURL:(NSURL*)destinationURL
             backgroundColor:(NSString*)backgroundColor
                   imagePath:(NSURL*)imagePath {
  if ((self = [super init])) {
    self.name = name;
    self.destinationURL = destinationURL;
    self.backgroundColor = backgroundColor;
    self.imagePath = imagePath;
  }
  return self;
}

- (instancetype)initWithTopSite:(const ntp_background_images::TopSite&)topSite {
  auto name = base::SysUTF8ToNSString(topSite.name);
  auto destinationURL =
      [NSURL URLWithString:base::SysUTF8ToNSString(topSite.destination_url)];
  auto backgroundColor = base::SysUTF8ToNSString(topSite.background_color);
  auto imagePath = [NSURL
      fileURLWithPath:base::SysUTF8ToNSString(topSite.image_file.value())];
  return [self initWithName:name
             destinationURL:destinationURL
            backgroundColor:backgroundColor
                  imagePath:imagePath];
}

@end
