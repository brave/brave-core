/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>
#import "ads.mojom.objc.h"

typedef NS_ENUM(NSInteger, BATAdNotificationEventType) {
  BATAdNotificationEventTypeServed,   // = ads::AdNotificationEventType::kServed
  BATAdNotificationEventTypeViewed,   // = ads::AdNotificationEventType::kViewed
  BATAdNotificationEventTypeClicked,  // =
                                      // ads::AdNotificationEventType::kClicked
  BATAdNotificationEventTypeDismissed,  // =
                                        // ads::AdNotificationEventType::kDismissed
  BATAdNotificationEventTypeTimedOut  // =
                                      // ads::AdNotificationEventType::kTimedOut
} NS_SWIFT_NAME(AdNotificationEventType);

typedef NS_ENUM(NSInteger, BATInlineContentAdEventType) {
  BATInlineContentAdEventTypeServed,  // =
                                      // ads::InlineContentAdEventType::kServed
  BATInlineContentAdEventTypeViewed,  // =
                                      // ads::InlineContentAdEventType::kViewed
  BATInlineContentAdEventTypeClicked  // =
                                      // ads::InlineContentAdEventType::kClicked
} NS_SWIFT_NAME(InlineContentAdEventType);

typedef NS_ENUM(NSInteger, BATNewTabPageAdEventType) {
  BATNewTabPageAdEventTypeServed,  // = ads::NewTabPageAdEventType::kServed
  BATNewTabPageAdEventTypeViewed,  // = ads::NewTabPageAdEventType::kViewed
  BATNewTabPageAdEventTypeClicked  // = ads::NewTabPageAdEventType::kClicked
} NS_SWIFT_NAME(NewTabPageAdEventType);

typedef NS_ENUM(NSInteger, BATPromotedContentAdEventType) {
  BATPromotedContentAdEventTypeServed,  // =
                                        // ads::PromotedContentAdEventType::kServed
  BATPromotedContentAdEventTypeViewed,  // =
                                        // ads::PromotedContentAdEventType::kViewed
  BATPromotedContentAdEventTypeClicked  // =
                                        // ads::PromotedContentAdEventType::kClicked
} NS_SWIFT_NAME(PromotedContentAdEventType);

NS_ASSUME_NONNULL_BEGIN

@class BATAdNotification, BATBraveAds, BATBraveLedger;

OBJC_EXPORT
NS_SWIFT_NAME(BraveAdsNotificationHandler)
@protocol BATBraveAdsNotificationHandler
@required
/// Determine whether or not the client can currently show notifications
/// to the user.
- (BOOL)shouldShowNotifications;
/// Show the given notification to the user (or add it to the queue)
- (void)showNotification:(BATAdNotification *)notification;
/// Remove a pending notification from the queue or remove an already shown
/// notification from view
- (void)clearNotificationWithIdentifier:(NSString *)identifier;
@end

OBJC_EXPORT 
NS_SWIFT_NAME(BraveAds)
@interface BATBraveAds : NSObject

@property (nonatomic, weak) BATBraveLedger *ledger;

/// The notifications handler.
///
/// @see BATSystemNotificationsHandler
@property (nonatomic, weak, nullable) id<BATBraveAdsNotificationHandler> notificationsHandler;

#pragma mark - Global

/// Whether or not a given locale is supported. The locale should be a standard
/// locale identifier, i.e. "en_US"
+ (BOOL)isSupportedLocale:(NSString *)locale;

/// Whether or not a given locale is newly supported. The locale should be a
/// standard locale identifier, i.e. "en_US"
+ (BOOL)isNewlySupportedLocale:(NSString *)locale;

/// Whether or not the users current locale (by `NSLocale`) is supported
+ (BOOL)isCurrentLocaleSupported;

/// Whether or not to use staging servers. Defaults to false
@property (nonatomic, class, getter=isDebug) BOOL debug;
/// The environment that ads is communicating with. See ledger's BATEnvironment
/// for appropriate values.
@property (nonatomic, class) int environment;
/// System info
@property (nonatomic, class) BATBraveAdsSysInfo *sysInfo;
/// The build channel that ads is configured for
@property (nonatomic, class) BATBraveAdsBuildChannel *buildChannel;

#pragma mark - Initialization / Shutdown

/// Initializes the ads service if ads is enabled
- (void)initializeIfAdsEnabled;

/// Shuts down the ads service if its running
- (void)shutdown:(nullable void (^)())completion;

/// Whether or not the ads service is running
- (BOOL)isAdsServiceRunning;

#pragma mark - Configuration

/// The max number of ads the user can see in an hour
@property (nonatomic, assign) NSInteger numberOfAllowableAdsPerHour NS_SWIFT_NAME(adsPerHour);

/// Whether or not the user has opted out of subdivision ad targeting
@property (nonatomic, assign, getter=shouldAllowSubdivisionTargeting) BOOL allowSubdivisionTargeting;

/// Selected ads subdivision targeting option
@property (nonatomic, copy) NSString * subdivisionTargetingCode;

/// Automatically detected ads subdivision targeting code
@property (nonatomic, copy) NSString * autoDetectedSubdivisionTargetingCode;

/// Remove all cached history (should be called when the user clears their browser history)
- (void)removeAllHistory:(void (^)(BOOL))completion;

#pragma mark - Confirmations

#pragma mark - Notificiations

- (nullable BATAdNotification *)adsNotificationForIdentifier:(NSString *)identifier;

#pragma mark - History

/// Get a list of dates of when the user has viewed ads
- (NSArray<NSDate *> *)getAdsHistoryDates;

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
- (void)reportMediaStartedWithTabId:(NSInteger)tabId NS_SWIFT_NAME(reportMediaStarted(tabId:));

/// Report that media has stopped on a tab with a given id
- (void)reportMediaStoppedWithTabId:(NSInteger)tabId NS_SWIFT_NAME(reportMediaStopped(tabId:));

/// Report that a tab with a given id was updated
- (void)reportTabUpdated:(NSInteger)tabId url:(NSURL *)url isSelected:(BOOL)isSelected isPrivate:(BOOL)isPrivate;

/// Report that a tab with a given id was closed by the user
- (void)reportTabClosedWithTabId:(NSInteger)tabId NS_SWIFT_NAME(reportTabClosed(tabId:));

/// Report that an ad notification event type was triggered for a given id
- (void)reportAdNotificationEvent:(NSString *)uuid
                        eventType:(BATAdNotificationEventType)eventType;

/// Get inline content ad for a given size
- (void)getInlineContentAd:(NSString* size,
                            void (^)(BOOL success,
                                     NSString* title,
                                     NSString* description,
                                     NSString* imageUrl,
                                     NSString* ctaText))completion
    NS_SWIFT_NAME(getInlineContentAd(_:));

/// Report that an inline content ad event type was triggered for a given id
- (void)reportInlineContentAdEvent:(NSString*)uuid
                creativeInstanceId:(NSString*)creativeInstanceId
                         eventType:(BATInlineContentAdEventType)eventType;

/// Report that a new tab page ad event type was triggered for a given id
- (void)reportNewTabPageAdEvent:(NSString *)wallpaperId
             creativeInstanceId:(NSString *)creativeInstanceId
                      eventType:(BATNewTabPageAdEventType)eventType;

/// Report that a promoted content ad event type was triggered for a given id
- (void)reportPromotedContentAdEvent:(NSString *)uuid
                  creativeInstanceId:(NSString *)creativeInstanceId
                           eventType:(BATPromotedContentAdEventType)eventType;

/// Reconcile ad rewards with server
- (void)reconcileAdRewards;

/// Get the number of ads received and the estimated earnings of viewing said ads for this cycle
- (void)detailsForCurrentCycle:(void (^)(NSInteger adsReceived, double estimatedEarnings, NSDate * _Nullable nextPaymentDate))completion NS_SWIFT_NAME(detailsForCurrentCycle(_:));

/// Toggle that the user liked the given ad and more like it should be shown
- (void)toggleThumbsUpForAd:(NSString *)creativeInstanceId
              creativeSetID:(NSString *)creativeSetID;

/// Toggle that the user disliked the given ad and it shouldn't be shown again
- (void)toggleThumbsDownForAd:(NSString *)creativeInstanceId
                creativeSetID:(NSString *)creativeSetID;

#pragma mark -

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithStateStoragePath:(NSString *)path;

@end

NS_ASSUME_NONNULL_END
