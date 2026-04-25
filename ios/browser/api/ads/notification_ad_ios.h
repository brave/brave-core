/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#ifndef BRAVE_IOS_BROWSER_API_ADS_NOTIFICATION_AD_IOS_H_
#define BRAVE_IOS_BROWSER_API_ADS_NOTIFICATION_AD_IOS_H_

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(NotificationAd)
@interface NotificationAdIOS : NSObject
@property(nonatomic, readonly) NSString* placementID;
@property(nonatomic, readonly) NSString* creativeInstanceID;
@property(nonatomic, readonly) NSString* creativeSetID;
@property(nonatomic, readonly) NSString* campaignID;
@property(nonatomic, readonly) NSString* advertiserID;
@property(nonatomic, readonly) NSString* segment;
@property(nonatomic, readonly) NSString* title;
@property(nonatomic, readonly) NSString* body;
@property(nonatomic, readonly) NSString* targetURL;
@end

OBJC_EXPORT
@interface NotificationAdIOS (MyFirstAd)
+ (instancetype)customAdWithTitle:(NSString*)title
                             body:(NSString*)body
                              url:(NSString*)url
    NS_SWIFT_NAME(customAd(title:body:url:));
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_ADS_NOTIFICATION_AD_IOS_H_
