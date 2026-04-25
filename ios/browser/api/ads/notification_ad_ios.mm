/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "notification_ad_ios.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface NotificationAdIOS ()
@property(nonatomic, copy) NSString* placementID;
@property(nonatomic, copy) NSString* creativeInstanceID;
@property(nonatomic, copy) NSString* creativeSetID;
@property(nonatomic, copy) NSString* campaignID;
@property(nonatomic, copy) NSString* advertiserID;
@property(nonatomic, copy) NSString* segment;
@property(nonatomic, copy) NSString* title;
@property(nonatomic, copy) NSString* body;
@property(nonatomic, copy) NSString* targetURL;
@end

@implementation NotificationAdIOS

- (instancetype)initWithNotificationAdInfo:
    (const brave_ads::NotificationAdInfo&)info {
  if ((self = [super init])) {
    self.placementID = base::SysUTF8ToNSString(info.placement_id);
    self.creativeInstanceID =
        base::SysUTF8ToNSString(info.creative_instance_id);
    self.creativeSetID = base::SysUTF8ToNSString(info.creative_set_id);
    self.campaignID = base::SysUTF8ToNSString(info.campaign_id);
    self.advertiserID = base::SysUTF8ToNSString(info.advertiser_id);
    self.segment = base::SysUTF8ToNSString(info.segment);
    self.title = base::SysUTF8ToNSString(info.title);
    self.body = base::SysUTF8ToNSString(info.body);
    self.targetURL = base::SysUTF8ToNSString(info.target_url.spec());
  }
  return self;
}

@end

@implementation NotificationAdIOS (MyFirstAd)
+ (instancetype)customAdWithTitle:(NSString*)title
                             body:(NSString*)body
                              url:(NSString*)url {
  NotificationAdIOS* notification = [[NotificationAdIOS alloc] init];
  notification.title = title;
  notification.body = body;
  notification.targetURL = url;
  return notification;
}
@end
