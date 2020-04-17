/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATAdNotification.h"
#include "bat/ads/ad_notification_info.h"
#import <map>

@interface BATAdNotification ()
@property (nonatomic, copy) NSString *uuid;
@property (nonatomic, copy) NSString *parentUuid;
@property (nonatomic, copy) NSString *creativeInstanceID;
@property (nonatomic, copy) NSString *creativeSetID;
@property (nonatomic, copy) NSString *category;
@property (nonatomic, copy) NSString *title;
@property (nonatomic, copy) NSString *body;
@property (nonatomic, copy) NSURL *targetURL;
@property (nonatomic, copy) NSString *geoTarget;
@end

@implementation BATAdNotification

- (instancetype)initWithNotificationInfo:(const ads::AdNotificationInfo&)info
{
  if ((self = [super init])) {
    self.uuid = [NSString stringWithUTF8String:info.uuid.c_str()];
    self.parentUuid = [NSString stringWithUTF8String:info.parent_uuid.c_str()];
    self.creativeInstanceID = [NSString stringWithUTF8String:info.creative_instance_id.c_str()];
    self.creativeSetID = [NSString stringWithUTF8String:info.creative_set_id.c_str()];
    self.category = [NSString stringWithUTF8String:info.category.c_str()];
    self.title = [NSString stringWithUTF8String:info.title.c_str()];
    self.body = [NSString stringWithUTF8String:info.body.c_str()];
    self.targetURL = [NSURL URLWithString:[NSString stringWithUTF8String:info.target_url.c_str()]];
    self.geoTarget = [NSString stringWithUTF8String:info.geo_target.c_str()];
  }
  return self;
}

@end

@implementation BATAdNotification (MyFirstAd)
+ (instancetype)customAdWithTitle:(NSString *)title body:(NSString *)body url:(NSURL *)url
{
  BATAdNotification *notification = [[BATAdNotification alloc] init];
  notification.title = title;
  notification.body = body;
  notification.targetURL = url;
  return notification;
}
@end
