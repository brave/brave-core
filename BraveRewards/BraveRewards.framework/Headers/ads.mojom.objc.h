/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif



typedef NS_ENUM(NSInteger, BATBraveAdsEnvironment) {
  BATBraveAdsEnvironmentStaging = 0,
  BATBraveAdsEnvironmentProduction,
  BATBraveAdsEnvironmentDevelopment,
} NS_SWIFT_NAME(BraveAdsEnvironment);


typedef NS_ENUM(NSInteger, BATBraveAdsAdNotificationEventType) {
  BATBraveAdsAdNotificationEventTypeKviewed = 0,
  BATBraveAdsAdNotificationEventTypeKclicked,
  BATBraveAdsAdNotificationEventTypeKdismissed,
  BATBraveAdsAdNotificationEventTypeKtimedout,
} NS_SWIFT_NAME(BraveAdsAdNotificationEventType);


typedef NS_ENUM(NSInteger, BATBraveAdsUrlRequestMethod) {
  BATBraveAdsUrlRequestMethodGet = 0,
  BATBraveAdsUrlRequestMethodPut,
  BATBraveAdsUrlRequestMethodPost,
} NS_SWIFT_NAME(BraveAdsUrlRequestMethod);



@class BATBraveAdsBuildChannel, BATBraveAdsUrlRequest, BATBraveAdsUrlResponse;

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(BraveAdsBuildChannel)
@interface BATBraveAdsBuildChannel : NSObject <NSCopying>
@property (nonatomic) bool isRelease;
@property (nonatomic, copy) NSString * name;
@end

NS_SWIFT_NAME(BraveAdsUrlRequest)
@interface BATBraveAdsUrlRequest : NSObject <NSCopying>
@property (nonatomic, copy) NSString * url;
@property (nonatomic, copy) NSArray<NSString *> * headers;
@property (nonatomic, copy) NSString * content;
@property (nonatomic, copy) NSString * contentType;
@property (nonatomic) BATBraveAdsUrlRequestMethod method;
@end

NS_SWIFT_NAME(BraveAdsUrlResponse)
@interface BATBraveAdsUrlResponse : NSObject <NSCopying>
@property (nonatomic, copy) NSString * url;
@property (nonatomic) int32_t statusCode;
@property (nonatomic, copy) NSString * body;
@property (nonatomic, copy) NSDictionary<NSString *, NSString *> * headers;
@end

NS_ASSUME_NONNULL_END