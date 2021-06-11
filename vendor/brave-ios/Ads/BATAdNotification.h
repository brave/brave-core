/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(AdsNotification)
@interface BATAdNotification : NSObject
@property (nonatomic, readonly, copy) NSString *uuid;
@property (nonatomic, readonly, copy) NSString *creativeInstanceID;
@property (nonatomic, readonly, copy) NSString *creativeSetID;
@property (nonatomic, readonly, copy) NSString *campaignID;
@property (nonatomic, readonly, copy) NSString *advertiserID;
@property (nonatomic, readonly, copy) NSString *segment;
@property (nonatomic, readonly, copy) NSString *title;
@property (nonatomic, readonly, copy) NSString *body;
@property (nonatomic, readonly, copy) NSString *targetURL;
@end

OBJC_EXPORT 
@interface BATAdNotification (MyFirstAd)
+ (instancetype)customAdWithTitle:(NSString *)title
                             body:(NSString *)body
                              url:(NSString *)url NS_SWIFT_NAME(customAd(title:body:url:));
@end

NS_ASSUME_NONNULL_END
