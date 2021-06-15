/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATInlineContentAd.h"
#include "bat/ads/inline_content_ad_info.h"

@interface BATInlineContentAd ()
@property(nonatomic, copy) NSString* uuid;
@property(nonatomic, copy) NSString* creativeInstanceID;
@property(nonatomic, copy) NSString* creativeSetID;
@property(nonatomic, copy) NSString* campaignID;
@property(nonatomic, copy) NSString* advertiserID;
@property(nonatomic, copy) NSString* segment;
@property(nonatomic, copy) NSString* title;
@property(nonatomic, copy) NSString* message;
@property(nonatomic, copy) NSString* imageURL;
@property(nonatomic, copy) NSString* dimensions;
@property(nonatomic, copy) NSString* ctaText;
@property(nonatomic, copy) NSString* targetURL;
@end

@implementation BATInlineContentAd

- (instancetype)initWithInlineContentAdInfo:
    (const ads::InlineContentAdInfo&)info {
  if ((self = [super init])) {
    self.uuid = [NSString stringWithUTF8String:info.uuid.c_str()];
    self.creativeInstanceID =
        [NSString stringWithUTF8String:info.creative_instance_id.c_str()];
    self.creativeSetID =
        [NSString stringWithUTF8String:info.creative_set_id.c_str()];
    self.campaignID = [NSString stringWithUTF8String:info.campaign_id.c_str()];
    self.advertiserID =
        [NSString stringWithUTF8String:info.advertiser_id.c_str()];
    self.segment = [NSString stringWithUTF8String:info.segment.c_str()];
    self.title = [NSString stringWithUTF8String:info.title.c_str()];
    self.message = [NSString stringWithUTF8String:info.description.c_str()];
    self.imageURL = [NSString stringWithUTF8String:info.image_url.c_str()];
    self.dimensions = [NSString stringWithUTF8String:info.dimensions.c_str()];
    self.ctaText = [NSString stringWithUTF8String:info.cta_text.c_str()];
    self.targetURL = [NSString stringWithUTF8String:info.target_url.c_str()];
  }
  return self;
}

@end
