/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATAdsNotification.h"
#include "bat/ads/ad_notification_info.h"
#import <map>

@interface BATAdsNotification ()
@property (nonatomic, copy) NSString *id;
@property (nonatomic, copy) NSString *creativeSetID;
@property (nonatomic, copy) NSString *category;
@property (nonatomic, copy) NSString *advertiser;
@property (nonatomic, copy) NSString *text;
@property (nonatomic, copy) NSURL *url;
@property (nonatomic, copy) NSString *uuid;
@property (nonatomic) BATAdsConfirmationType confirmationType;
@end

@implementation BATAdsNotification

- (instancetype)initWithNotificationInfo:(const ads::AdNotificationInfo&)info
{
  if ((self = [super init])) {
    self.id = [NSString stringWithUTF8String:info.id.c_str()];
    self.creativeSetID = [NSString stringWithUTF8String:info.creative_set_id.c_str()];
    self.category = [NSString stringWithUTF8String:info.category.c_str()];
    self.advertiser = [NSString stringWithUTF8String:info.advertiser.c_str()];
    self.text = [NSString stringWithUTF8String:info.text.c_str()];
    self.url = [NSURL URLWithString:[NSString stringWithUTF8String:info.url.c_str()]];
    self.uuid = [NSString stringWithUTF8String:info.uuid.c_str()];
    // FIXME: Move to ConfirmationsType class here, instead of just casting its enum value
    self.confirmationType = (BATAdsConfirmationType)info.type.value();
  }
  return self;
}

@end

@implementation BATAdsNotification (MyFirstAd)
+ (instancetype)customAdWithTitle:(NSString *)title body:(NSString *)body url:(NSURL *)url
{
  BATAdsNotification *notification = [[BATAdsNotification alloc] init];
  notification.advertiser = title;
  notification.text = body;
  notification.url = url;
  return notification;
}
@end
