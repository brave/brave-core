/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_ADS_BRAVE_ADS_H_
#define BRAVE_IOS_BROWSER_API_ADS_BRAVE_ADS_H_

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>
#import "brave_ads.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

@class NotificationAdIOS, InlineContentAdIOS;

OBJC_EXPORT
@protocol BraveAdsNotificationHandler
@required
/// Returns `true` if notification ads can be shown.
- (BOOL)canShowNotificationAds;
/// Show notification `ad`.
- (void)showNotificationAd:(NotificationAdIOS*)ad;
/// Close the notification ad for the specified `placement_id`.
- (void)closeNotificationAd:(NSString*)placementId;
@end

OBJC_EXPORT
@protocol BraveAdsCaptchaHandler
@required
/// Handle an adaptive captcha request for a given payment ID and captcha ID
- (void)handleAdaptiveCaptchaForPaymentId:(NSString*)paymentId
                                captchaId:(NSString*)captchaId;
@end

OBJC_EXPORT
@interface BraveAds : NSObject

/// The notifications handler.
///
/// @see BraveAdsNotificationHandler
@property(nonatomic, weak, nullable) id<BraveAdsNotificationHandler>
    notificationsHandler;

/// An object to handle adaptive captcha requests
///
/// @see BraveAdsCaptchaHandler
@property(nonatomic, weak, nullable) id<BraveAdsCaptchaHandler> captchaHandler;

#pragma mark -

/// Returns `true` if ads are supported for the user's current country otherwise
/// returns `false`.
+ (BOOL)isSupportedRegion;

/// Returns `true` if the ads service is running otherwise returns `false`.
- (BOOL)isServiceRunning;

/// Returns `true` if always run the ads service, even if Brave Private Ads are
/// disabled.
+ (BOOL)shouldAlwaysRunService;

/// Returns `true` if search result ads are supported.
+ (BOOL)shouldSupportSearchResultAds;

/// Returns `true` if should show Sponsored Images & Videos option in settings.
/// This function will be deprecated once Sponsored Video is available globally.
- (BOOL)shouldShowSponsoredImagesAndVideosSetting;

/// Returns `true` if the user opted-in to search result ads.
- (BOOL)isOptedInToSearchResultAds;

/// Used to notify the ads service that the user has opted-in/opted-out to
/// Brave News.
- (void)notifyBraveNewsIsEnabledPreferenceDidChange:(BOOL)isEnabled;

/// Whether or not Brave Ads is enabled and the user should receive
/// notification-style ads and be rewarded for it
@property(nonatomic, assign, getter=isEnabled)
    BOOL enabled NS_SWIFT_NAME(isEnabled);

#pragma mark - Initialization / Shutdown

- (void)initServiceWithSysInfo:(BraveAdsSysInfo*)sysInfo
              buildChannelInfo:(BraveAdsBuildChannelInfo*)buildChannelInfo
                    walletInfo:(nullable BraveAdsWalletInfo*)walletInfo
                    completion:(void (^)(bool))completion;

/// Returns false if the ad service is already running.
- (void)shutdownService:(nullable void (^)())completion;

#pragma mark - Ads

// See `components/brave_ads/core/internal/ads_impl.h`.

- (void)getStatementOfAccounts:
    (void (^)(NSInteger adsReceived,
              double estimatedEarnings,
              NSDate* _Nullable nextPaymentDate))completion;

- (void)maybeServeInlineContentAd:(NSString*)dimensions
                       completion:
                           (void (^)(NSString* dimensions,
                                     InlineContentAdIOS* _Nullable))completion;

- (void)triggerInlineContentAdEvent:(NSString*)placementId
                 creativeInstanceId:(NSString*)creativeInstanceId
                          eventType:(BraveAdsInlineContentAdEventType)eventType
                         completion:(void (^)(BOOL success))completion;

- (void)triggerNewTabPageAdEvent:(NSString*)wallpaperId
              creativeInstanceId:(NSString*)creativeInstanceId
                       eventType:(BraveAdsNewTabPageAdEventType)eventType
                      completion:(void (^)(BOOL success))completion;

- (void)maybeGetNotificationAd:(NSString*)identifier
                    completion:
                        (void (^)(NotificationAdIOS* _Nullable))completion;

- (void)triggerNotificationAdEvent:(NSString*)placementId
                         eventType:(BraveAdsNotificationAdEventType)eventType
                        completion:(void (^)(BOOL success))completion;

- (void)triggerPromotedContentAdEvent:(NSString*)placementId
                   creativeInstanceId:(NSString*)creativeInstanceId
                            eventType:
                                (BraveAdsPromotedContentAdEventType)eventType
                           completion:(void (^)(BOOL success))completion;

- (void)triggerSearchResultAdClickedEvent:(NSString*)placementId
                               completion:(void (^)(BOOL success))completion;

- (void)triggerSearchResultAdEvent:
            (BraveAdsCreativeSearchResultAdInfo*)searchResultAd
                         eventType:(BraveAdsSearchResultAdEventType)eventType
                        completion:(void (^)(BOOL success))completion;

- (void)purgeOrphanedAdEventsForType:(BraveAdsAdType)adType
                          completion:(void (^)(BOOL success))completion;

- (void)clearData:(void (^)())completion;

#pragma mark - Ads client notifier

// See `components/brave_ads/core/public/ads_client/ads_client_notifier.h`.

- (void)notifyRewardsWalletDidUpdate:(NSString*)paymentId
                          base64Seed:(NSString*)base64Seed;

- (void)notifyTabTextContentDidChange:(NSInteger)tabId
                        redirectChain:(NSArray<NSURL*>*)redirectChain
                                 text:(NSString*)text;

- (void)notifyTabHtmlContentDidChange:(NSInteger)tabId
                        redirectChain:(NSArray<NSURL*>*)redirectChain
                                 html:(NSString*)html;

- (void)notifyTabDidStartPlayingMedia:(NSInteger)tabId;

- (void)notifyTabDidStopPlayingMedia:(NSInteger)tabId;

- (void)notifyTabDidChange:(NSInteger)tabId
             redirectChain:(NSArray<NSURL*>*)redirectChain
           isNewNavigation:(BOOL)isNewNavigation
               isRestoring:(BOOL)isRestoring
                isSelected:(BOOL)isSelected;

- (void)notifyTabDidLoad:(NSInteger)tabId
          httpStatusCode:(NSInteger)httpStatusCode;

- (void)notifyDidCloseTab:(NSInteger)tabId;

#pragma mark -

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithStateStoragePath:(NSString*)path;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_ADS_BRAVE_ADS_H_
