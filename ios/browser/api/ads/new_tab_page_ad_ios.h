/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_ADS_NEW_TAB_PAGE_AD_IOS_H_
#define BRAVE_IOS_BROWSER_API_ADS_NEW_TAB_PAGE_AD_IOS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(NewTabPageAd)
@interface NewTabPageAdIOS : NSObject
@property(nonatomic, readonly, copy) NSString* placementID;
@property(nonatomic, readonly, copy) NSString* creativeInstanceID;
@property(nonatomic, readonly, copy) NSString* creativeSetID;
@property(nonatomic, readonly, copy) NSString* campaignID;
@property(nonatomic, readonly, copy) NSString* advertiserID;
@property(nonatomic, readonly, copy) NSString* segment;
@property(nonatomic, readonly, copy) NSString* targetURL;
@property(nonatomic, readonly, copy) NSString* companyName;
@property(nonatomic, readonly, copy) NSString* alt;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_ADS_NEW_TAB_PAGE_AD_IOS_H_
