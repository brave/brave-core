/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#ifndef BRAVE_IOS_BROWSER_API_ADS_INLINE_CONTENT_AD_IOS_H_
#define BRAVE_IOS_BROWSER_API_ADS_INLINE_CONTENT_AD_IOS_H_

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(InlineContentAd)
@interface InlineContentAdIOS : NSObject
@property(nonatomic, readonly, copy) NSString* uuid;
@property(nonatomic, readonly, copy) NSString* creativeInstanceID;
@property(nonatomic, readonly, copy) NSString* creativeSetID;
@property(nonatomic, readonly, copy) NSString* campaignID;
@property(nonatomic, readonly, copy) NSString* advertiserID;
@property(nonatomic, readonly, copy) NSString* segment;
@property(nonatomic, readonly, copy) NSString* title;
@property(nonatomic, readonly, copy) NSString* message;
@property(nonatomic, readonly, copy) NSString* imageURL;
@property(nonatomic, readonly, copy) NSString* dimensions;
@property(nonatomic, readonly, copy) NSString* ctaText;
@property(nonatomic, readonly, copy) NSString* targetURL;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_ADS_INLINE_CONTENT_AD_IOS_H_
