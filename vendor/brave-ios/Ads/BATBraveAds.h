/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>

typedef NS_ENUM(NSInteger, BATAdsNotificationEventType) {
  BATAdsNotificationEventTypeViewed,     // = ads::NotificationEventType::VIEWED
  BATAdsNotificationEventTypeClicked,    // = ads::NotificationEventType::CLICKED
  BATAdsNotificationEventTypeDismissed,  // = ads::NotificationEventType::DISMISSED
  BATAdsNotificationEventTypeTimedOut    // = ads::NotificationEventType::TIMEOUT
} NS_SWIFT_NAME(NotificationEventType);

NS_ASSUME_NONNULL_BEGIN

@class BATAdsNotification, BATBraveAds, BATBraveLedger;

NS_SWIFT_NAME(BraveAdsNotificationHandler)
@protocol BATBraveAdsNotificationHandler
@required
/// Determine whether or not the client can currently show notifications
/// to the user.
- (BOOL)shouldShowNotifications;
/// Show the given notification to the user (or add it to the queue)
- (void)showNotification:(BATAdsNotification *)notification;
/// Remove a pending notification from the queue or remove an already shown
/// notification from view
- (void)clearNotificationWithIdentifier:(NSString *)identifier;
@end

NS_SWIFT_NAME(BraveAds)
@interface BATBraveAds : NSObject

@property (nonatomic, weak) BATBraveLedger *ledger;

/// The notifications handler.
///
/// @see BATSystemNotificationsHandler
@property (nonatomic, weak, nullable) id<BATBraveAdsNotificationHandler> notificationsHandler;

#pragma mark - Global

+ (BOOL)isSupportedRegion:(NSString *)region;

/// Whether or not to use staging servers. Defaults to false
@property (nonatomic, class, getter=isDebug) BOOL debug;
/// Whether or not to use production servers. Defaults to true
@property (nonatomic, class, getter=isProduction) BOOL production;
/// Marks if this is being ran in a test environment. Defaults to false
@property (nonatomic, class, getter=isTesting) BOOL testing;

#pragma mark - Configuration

/// Whether or not Brave Ads is enabled
@property (nonatomic, assign, getter=isEnabled) BOOL enabled;

/// The max number of ads the user can see in an hour
@property (nonatomic, assign) NSInteger numberOfAllowableAdsPerHour NS_SWIFT_NAME(adsPerHour);

/// The max number of ads the user can see in a day
@property (nonatomic, assign) NSInteger numberOfAllowableAdsPerDay NS_SWIFT_NAME(adsPerDay);

/// The user model locales Brave Ads supports currently
@property (nonatomic, readonly) NSArray<NSString *> *userModelLanguages;

/// Remove all cached history (should be called when the user clears their browser history)
- (void)removeAllHistory:(void (^)(BOOL))completion;

/// Should be called when the user invokes "Show Sample Ad" on the Client; a Notification is then sent
/// to the Client for processing
- (void)serveSampleAd;

#pragma mark - Confirmations

// Should be called to inform Ads if Confirmations is ready
- (void)setConfirmationsIsReady:(BOOL)isReady;

#pragma mark - Notificiations

- (nullable BATAdsNotification *)adsNotificationForIdentifier:(NSString *)identifier;

#pragma mark - Reporting

/// Report that a page has loaded in the current browser tab, and the HTML is available for analysis
- (void)reportLoadedPageWithURL:(NSURL *)url html:(NSString *)html;

/// Report that media has started on a tab with a given id
- (void)reportMediaStartedWithTabId:(NSInteger)tabId NS_SWIFT_NAME(reportMediaStarted(tabId:));

/// Report that media has stopped on a tab with a given id
- (void)reportMediaStoppedWithTabId:(NSInteger)tabId NS_SWIFT_NAME(reportMediaStopped(tabId:));

/// Report that a tab with a given id was updated
- (void)reportTabUpdated:(NSInteger)tabId url:(NSURL *)url isSelected:(BOOL)isSelected isPrivate:(BOOL)isPrivate;

/// Report that a tab with a given id was closed by the user
- (void)reportTabClosedWithTabId:(NSInteger)tabId NS_SWIFT_NAME(reportTabClosed(tabId:));

/// Report that a notification event type was triggered for a given id
- (void)reportNotificationEvent:(NSString *)notificationId
                      eventType:(BATAdsNotificationEventType)eventType;

/// Toggle that the user liked the given ad and more like it should be shown
- (void)toggleThumbsUpForAd:(NSString *)identifier
              creativeSetID:(NSString *)creativeSetID;

/// Toggle that the user disliked the given ad and it shouldn't be shown again
- (void)toggleThumbsDownForAd:(NSString *)identifier
                creativeSetID:(NSString *)creativeSetID;

#pragma mark -

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithStateStoragePath:(NSString *)path;

@end

NS_ASSUME_NONNULL_END
