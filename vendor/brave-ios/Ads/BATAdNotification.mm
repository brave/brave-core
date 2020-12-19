/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATAdNotification.h"
#include "bat/ads/ad_notification_info.h"
#import <map>

@interface BATAdNotification ()
@property (nonatomic, copy) NSString *uuid;
@property (nonatomic, copy) NSString *creativeInstanceID;
@property (nonatomic, copy) NSString *creativeSetID;
@property (nonatomic, copy) NSString *campaignID;
@property (nonatomic, copy) NSString *segment;
@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy) NSString *body;
@property (nonatomic, copy) NSString *targetURL;
@end

@implementation BATAdNotification

- (instancetype)initWithNotificationInfo:(const ads::AdNotificationInfo &)info
{
  if ((self = [super init])) {
    self.uuid = [NSString stringWithUTF8String:info.uuid.c_str()];
    self.creativeInstanceID = [NSString stringWithUTF8String:info.creative_instance_id.c_str()];
    self.creativeSetID = [NSString stringWithUTF8String:info.creative_set_id.c_str()];
    self.campaignID = [NSString stringWithUTF8String:info.campaign_id.c_str()];
    self.segment = [NSString stringWithUTF8String:info.segment.c_str()];
    self.title = [NSString stringWithUTF8String:info.title.c_str()];
    self.body = [NSString stringWithUTF8String:info.body.c_str()];
    self.targetURL = [NSString stringWithUTF8String:info.target_url.c_str()];
  }
  return self;
}

@end

@implementation BATAdNotification (MyFirstAd)
+ (instancetype)customAdWithTitle:(NSString *)title body:(NSString *)body url:(NSString *)url
{
  BATAdNotification *notification = [[BATAdNotification alloc] init];
  notification.title = title;
  notification.body = body;
  notification.targetURL = url;
  return notification;
}
@end
