/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "rewards_notification.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation RewardsNotification

- (instancetype)initWithID:(NSString*)notificationID
                 dateAdded:(NSTimeInterval)dateAdded
                      kind:(RewardsNotificationKind)kind
                  userInfo:(NSDictionary*)userInfo {
  if ((self = [super init])) {
    self.id = notificationID;
    self.dateAdded = dateAdded;
    self.kind = kind;
    self.userInfo = userInfo;
    self.displayed = NO;
  }
  return self;
}

+ (BOOL)supportsSecureCoding {
  return YES;
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  if ((self = [super init])) {
    self.id = [aDecoder decodeObjectOfClass:NSString.class forKey:@"id"];
    self.dateAdded = [aDecoder decodeDoubleForKey:@"dateAdded"];
    self.kind = (RewardsNotificationKind)[aDecoder decodeIntegerForKey:@"kind"];
    self.userInfo =
        [aDecoder decodeObjectOfClass:NSDictionary.class forKey:@"userInfo"];
    self.displayed = [aDecoder decodeBoolForKey:@"displayed"];
  }
  return self;
}

- (void)encodeWithCoder:(NSCoder*)aCoder {
  [aCoder encodeObject:self.id forKey:@"id"];
  [aCoder encodeDouble:self.dateAdded forKey:@"dateAdded"];
  [aCoder encodeInteger:self.kind forKey:@"kind"];
  [aCoder encodeObject:self.userInfo forKey:@"userInfo"];
  [aCoder encodeBool:self.displayed forKey:@"displayed"];
}

@end
