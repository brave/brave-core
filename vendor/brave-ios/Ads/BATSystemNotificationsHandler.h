/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>

#import "BATBraveAds.h"

NS_ASSUME_NONNULL_BEGIN

/// A default system notifications handler for managing displaying Brave Ads.
///
/// If this class is set to the UNUserNotificationCenter's delegate (or receives
/// UNUserNotificationCenterDelegate methods via proxy) it will automatically
/// report notification events back to the ads lib
NS_SWIFT_NAME(SystemNotificationsHandler)
@interface BATSystemNotificationsHandler : NSObject <BATBraveAdsNotificationHandler, UNUserNotificationCenterDelegate>
/// An ad was tapped and a URL should be opened
@property (nonatomic, copy, nullable) void (^adTapped)(BATAdsNotification *);
/// The ads object
@property (nonatomic, readonly) BATBraveAds *ads;
/// Create a handler instance with the given ads instance.
///
/// @note This method automatically sets `notificationsHandler` on BATBraveAds
/// to itself
- (instancetype)initWithAds:(BATBraveAds *)ads;
/// Whether or not this class should handle UNUserNotificationCenterDelegate
/// methods given the request
///
/// Use this if you are _not_ setting the UNUserNotificationCenter's delegate to
/// an instance of this class. This will allow you to determine whether to
/// forward the delegate method to this object.
- (BOOL)shouldHandleNotificationRequest:(UNNotificationRequest *)request;
/// Requests the notification permissions
- (void)requestNotificationPermissions:(void (^)(BOOL granted))completion;
@end

NS_ASSUME_NONNULL_END
