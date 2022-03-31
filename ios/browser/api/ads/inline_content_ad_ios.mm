/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "inline_content_ad_ios.h"

#include "base/strings/sys_string_conversions.h"
#include "bat/ads/inline_content_ad_info.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface InlineContentAdIOS ()
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

@implementation InlineContentAdIOS

- (instancetype)initWithInlineContentAdInfo:
    (const ads::InlineContentAdInfo&)info {
  if ((self = [super init])) {
    self.uuid = base::SysUTF8ToNSString(info.uuid);
    self.creativeInstanceID =
        base::SysUTF8ToNSString(info.creative_instance_id);
    self.creativeSetID = base::SysUTF8ToNSString(info.creative_set_id);
    self.campaignID = base::SysUTF8ToNSString(info.campaign_id);
    self.advertiserID = base::SysUTF8ToNSString(info.advertiser_id);
    self.segment = base::SysUTF8ToNSString(info.segment);
    self.title = base::SysUTF8ToNSString(info.title);
    self.message = base::SysUTF8ToNSString(info.description);
    self.imageURL = base::SysUTF8ToNSString(info.image_url);
    self.dimensions = base::SysUTF8ToNSString(info.dimensions);
    self.ctaText = base::SysUTF8ToNSString(info.cta_text);
    self.targetURL = base::SysUTF8ToNSString(info.target_url);
  }
  return self;
}

@end
