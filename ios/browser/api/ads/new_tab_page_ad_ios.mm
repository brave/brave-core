/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/ads/new_tab_page_ad_ios.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

@interface NewTabPageAdIOS ()
@property(nonatomic, copy) NSString* placementID;
@property(nonatomic, copy) NSString* creativeInstanceID;
@property(nonatomic, copy) NSString* creativeSetID;
@property(nonatomic, copy) NSString* campaignID;
@property(nonatomic, copy) NSString* advertiserID;
@property(nonatomic, copy) NSString* segment;
@property(nonatomic, copy) NSString* targetURL;
@property(nonatomic, copy) NSString* companyName;
@property(nonatomic, copy) NSString* alt;
@end

@implementation NewTabPageAdIOS

- (instancetype)initWithNewTabPageAdInfo:
    (const brave_ads::NewTabPageAdInfo&)ad {
  if ((self = [super init])) {
    self.placementID = base::SysUTF8ToNSString(ad.placement_id);
    self.creativeInstanceID = base::SysUTF8ToNSString(ad.creative_instance_id);
    self.creativeSetID = base::SysUTF8ToNSString(ad.creative_set_id);
    self.campaignID = base::SysUTF8ToNSString(ad.campaign_id);
    self.advertiserID = base::SysUTF8ToNSString(ad.advertiser_id);
    self.segment = base::SysUTF8ToNSString(ad.segment);
    self.targetURL = base::SysUTF8ToNSString(ad.target_url.spec());
    self.companyName = base::SysUTF8ToNSString(ad.company_name);
    self.alt = base::SysUTF8ToNSString(ad.alt);
  }
  return self;
}

@end
