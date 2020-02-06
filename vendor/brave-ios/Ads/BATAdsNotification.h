/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, BATAdsConfirmationType) {
  BATAdsConfirmationTypeUnknown,  // = ads::ConfirmationType::kUnknown
  BATAdsConfirmationTypeClick,    // = ads::ConfirmationType::kClicked
  BATAdsConfirmationTypeDismiss,  // = ads::ConfirmationType::kDismissed
  BATAdsConfirmationTypeView,     // = ads::ConfirmationType::kViewed
  BATAdsConfirmationTypeLanded    // = ads::ConfirmationType::kLanded
} NS_SWIFT_NAME(ConfirmationType);

NS_SWIFT_NAME(AdsNotification)
@interface BATAdsNotification : NSObject
@property (nonatomic, readonly, copy) NSString *id;
@property (nonatomic, readonly, copy) NSString *creativeSetID;
@property (nonatomic, readonly, copy) NSString *category;
@property (nonatomic, readonly, copy) NSString *advertiser;
@property (nonatomic, readonly, copy) NSString *text;
@property (nonatomic, readonly, copy) NSURL *url;
@property (nonatomic, readonly, copy) NSString *uuid;
@property (nonatomic, readonly) BATAdsConfirmationType confirmationType;
@end

@interface BATAdsNotification (MyFirstAd)
+ (instancetype)customAdWithTitle:(NSString *)title
                             body:(NSString *)body
                              url:(NSURL *)url NS_SWIFT_NAME(customAd(title:body:url:));
@end

NS_ASSUME_NONNULL_END
