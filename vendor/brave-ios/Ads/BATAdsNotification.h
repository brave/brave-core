/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, BATAdsConfirmationType) {
  BATAdsConfirmationTypeUnknown,    // = ads::ConfirmationType::kUnknown
  BATAdsConfirmationTypeClick,      // = ads::ConfirmationType::kClicked
  BATAdsConfirmationTypeDismiss,    // = ads::ConfirmationType::kDismissed
  BATAdsConfirmationTypeView,       // = ads::ConfirmationType::kViewed
  BATAdsConfirmationTypeLanded,     // = ads::ConfirmationType::kLanded
  BATAdsConfirmationTypeFlagged,    // = ads::ConfirmationType::kFlagged
  BATAdsConfirmationTypeUpvoted,    // = ads::ConfirmationType::kUpvoted
  BATAdsConfirmationTypeDownvoted,  // = ads::ConfirmationType::kDownvoted
  BATAdsConfirmationTypeConversion  // = ads::ConfirmationType::kConversion
} NS_SWIFT_NAME(ConfirmationType);

NS_SWIFT_NAME(AdsNotification)
@interface BATAdsNotification : NSObject
@property (nonatomic, readonly, copy) NSString *uuid;
@property (nonatomic, readonly, copy) NSString *parentUuid;
@property (nonatomic, readonly, copy) NSString *creativeInstanceID;
@property (nonatomic, readonly, copy) NSString *creativeSetID;
@property (nonatomic, readonly, copy) NSString *category;
@property (nonatomic, readonly, copy) NSString *title;
@property (nonatomic, readonly, copy) NSString *body;
@property (nonatomic, readonly, copy) NSURL *targetURL;
@property (nonatomic, readonly) BATAdsConfirmationType confirmationType;
@end

@interface BATAdsNotification (MyFirstAd)
+ (instancetype)customAdWithTitle:(NSString *)title
                             body:(NSString *)body
                              url:(NSURL *)url NS_SWIFT_NAME(customAd(title:body:url:));
@end

NS_ASSUME_NONNULL_END
