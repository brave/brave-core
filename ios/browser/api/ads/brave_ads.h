/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_ADS_BRAVE_ADS_H_
#define BRAVE_IOS_BROWSER_API_ADS_BRAVE_ADS_H_

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>
#import "ads.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

@class AdNotificationIOS, InlineContentAdIOS;

OBJC_EXPORT
@protocol BraveAdsNotificationHandler
@required
/// Determine whether or not the client can currently show notifications
/// to the user.
- (BOOL)shouldShowNotifications;
/// Show the given notification to the user (or add it to the queue)
- (void)showNotification:(AdNotificationIOS*)notification;
/// Remove a pending notification from the queue or remove an already shown
/// notification from view
- (void)clearNotificationWithIdentifier:(NSString*)identifier;
@end

OBJC_EXPORT
@interface BraveAds : NSObject

/// The notifications handler.
///
/// @see BraveAdsNotificationHandler
@property(nonatomic, weak, nullable) id<BraveAdsNotificationHandler>
    notificationsHandler;

#pragma mark - Global

/// Whether or not a given locale is supported. The locale should be a standard
/// locale identifier, i.e. "en_US"
+ (BOOL)isSupportedLocale:(NSString*)locale;

/// Whether or not the users current locale (by `NSLocale`) is supported
+ (BOOL)isCurrentLocaleSupported;

/// Whether or not to use staging servers. Defaults to false
@property(nonatomic, class, getter=isDebug) BOOL debug;
/// The environment that ads is communicating with.
@property(nonatomic, class) AdsEnvironment environment;
/// System info
@property(nonatomic, class) AdsSysInfo* sysInfo;
/// The build channel that ads is configured for
@property(nonatomic, class) AdsBuildChannel* buildChannel;

#pragma mark - Initialization / Shutdown

/// Initializes the ads service
- (void)initialize:(void (^)(bool success))completion;

/// Shuts down the ads service if its running
- (void)shutdown:(nullable void (^)())completion;

/// Whether or not the ads service is running
- (BOOL)isAdsServiceRunning;

/// Update the ad library with the users current wallet
- (void)updateWalletInfo:(NSString*)paymentId base64Seed:(NSString*)base64Seed;

#pragma mark - Configuration

/// Whether or not Brave Ads is enabled and the user should receive
/// notification-style ads and be rewarded for it
@property(nonatomic, assign, getter=isEnabled)
    BOOL enabled NS_SWIFT_NAME(isEnabled);

/// The max number of ads the user can see in an hour
@property(nonatomic, assign)
    NSInteger numberOfAllowableAdsPerHour NS_SWIFT_NAME(adsPerHour);

/// Whether or not the user has opted out of subdivision ad targeting
@property(nonatomic, assign, getter=shouldAllowSubdivisionTargeting)
    BOOL allowSubdivisionTargeting;

/// Selected ads subdivision targeting option
@property(nonatomic, copy) NSString* subdivisionTargetingCode;

/// Automatically detected ads subdivision targeting code
@property(nonatomic, copy) NSString* autoDetectedSubdivisionTargetingCode;

/// Remove all cached history (should be called when the user clears their
/// browser history)
- (void)removeAllHistory:(void (^)(BOOL))completion;

#pragma mark - Notificiations

- (nullable AdNotificationIOS*)adsNotificationForIdentifier:
    (NSString*)identifier;

#pragma mark - History

/// Get a list of dates of when the user has viewed ads
- (NSArray<NSDate*>*)getAdsHistoryDates;

/// Return true if the user has viewed ads in the previous cycle/month
- (BOOL)hasViewedAdsInPreviousCycle;

#pragma mark - Reporting

/// Report that a page has loaded in the current browser tab, and the html and
/// inner text within the page loaded for classification
- (void)reportLoadedPageWithURL:(NSURL*)url
             redirectedFromURLs:(NSArray<NSURL*>*)redirectionURLs
                           html:(NSString*)html
                      innerText:(NSString*)text
                          tabId:(NSInteger)tabId;

/// Report that media has started on a tab with a given id
- (void)reportMediaStartedWithTabId:(NSInteger)tabId
    NS_SWIFT_NAME(reportMediaStarted(tabId:));

/// Report that media has stopped on a tab with a given id
- (void)reportMediaStoppedWithTabId:(NSInteger)tabId
    NS_SWIFT_NAME(reportMediaStopped(tabId:));

/// Report that a tab with a given id was updated
- (void)reportTabUpdated:(NSInteger)tabId
                     url:(NSURL*)url
              isSelected:(BOOL)isSelected
               isPrivate:(BOOL)isPrivate;

/// Report that a tab with a given id was closed by the user
- (void)reportTabClosedWithTabId:(NSInteger)tabId
    NS_SWIFT_NAME(reportTabClosed(tabId:));

/// Report that an ad notification event type was triggered for a given id
- (void)reportAdNotificationEvent:(NSString*)placementId
                        eventType:(AdsAdNotificationEventType)eventType;

/// Get inline content ad for the given dimensions
- (void)inlineContentAdsWithDimensions:(NSString*)dimensions
                            completion:(void (^)(BOOL success,
                                                 NSString* dimensions,
                                                 InlineContentAdIOS*))completion
    NS_SWIFT_NAME(inlineContentAds(dimensions:completion:));

/// Report that an inline content ad event type was triggered for a given id
- (void)reportInlineContentAdEvent:(NSString*)placementId
                creativeInstanceId:(NSString*)creativeInstanceId
                         eventType:(AdsInlineContentAdEventType)eventType;

/// Report that a new tab page ad event type was triggered for a given id
- (void)reportNewTabPageAdEvent:(NSString*)wallpaperId
             creativeInstanceId:(NSString*)creativeInstanceId
                      eventType:(AdsNewTabPageAdEventType)eventType;

/// Report that a promoted content ad event type was triggered for a given id
- (void)reportPromotedContentAdEvent:(NSString*)placementId
                  creativeInstanceId:(NSString*)creativeInstanceId
                           eventType:(AdsPromotedContentAdEventType)eventType;

/// Purge orphaned ad events for a given ad type
- (void)purgeOrphanedAdEvents:(AdsAdType)adType;

/// Get the number of ads received and the estimated earnings of viewing said
/// ads for this cycle
- (void)detailsForCurrentCycle:
    (void (^)(NSInteger adsReceived,
              double estimatedEarnings,
              NSDate* _Nullable nextPaymentDate))completion
    NS_SWIFT_NAME(detailsForCurrentCycle(_:));

/// Toggle that the user liked the given ad and advertiser and more like it
/// should be shown
- (void)toggleThumbsUpForAd:(NSString*)creativeInstanceId
               advertiserId:(NSString*)advertiserId;

/// Toggle that the user disliked the given ad and advertiser and it shouldn't
/// be shown again
- (void)toggleThumbsDownForAd:(NSString*)creativeInstanceId
                 advertiserId:(NSString*)advertiserId;

#pragma mark -

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithStateStoragePath:(NSString*)path;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_ADS_BRAVE_ADS_H_
