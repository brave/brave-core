/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATSystemNotificationsHandler.h"
#import "BATAdsNotification.h"

@interface BATSystemNotificationsHandler ()
@property (nonatomic) BATBraveAds *ads;
@end

@implementation BATSystemNotificationsHandler

- (instancetype)initWithAds:(BATBraveAds *)ads
{
  if ((self = [super init])) {
    self.ads = ads;
    self.ads.notificationsHandler = self;
  }
  return self;
}

- (void)requestNotificationPermissions:(void (^)(BOOL granted))completion
{
  const auto options = UNAuthorizationOptionAlert |
  UNAuthorizationOptionSound |
  UNAuthorizationOptionBadge;
  [[UNUserNotificationCenter currentNotificationCenter]
   requestAuthorizationWithOptions:options completionHandler:^(BOOL granted, NSError * _Nullable error) {
     completion(granted);
   }];
}

- (BOOL)shouldHandleNotificationRequest:(UNNotificationRequest *)request
{
  const auto identifier = request.identifier;
  return [self.ads adsNotificationForIdentifier:identifier] != nil;
}

#pragma mark - BATBraveAdsNotificationHandler

- (void)shouldShowNotifications:(void (^)(BOOL))completionHandler
{
  const auto center = [UNUserNotificationCenter currentNotificationCenter];
  [center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings * _Nonnull settings) {
    switch (settings.authorizationStatus) {
      case UNAuthorizationStatusAuthorized:
      case UNAuthorizationStatusProvisional: {
        BOOL canBeDeliveredVisibly = (settings.alertSetting == UNNotificationSettingEnabled ||
                                      settings.notificationCenterSetting == UNNotificationSettingEnabled ||
                                      settings.lockScreenSetting == UNNotificationSettingEnabled);
        completionHandler(canBeDeliveredVisibly);
        break;
      }
      case UNAuthorizationStatusNotDetermined:
        [self requestNotificationPermissions:completionHandler];
        break;
      case UNAuthorizationStatusDenied:
        completionHandler(false);
        break;
    }
  }];
}

- (void)showNotification:(BATAdsNotification *)notification
{
  auto trigger = [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:1 repeats:NO];
  auto content = [[UNMutableNotificationContent alloc] init];
  content.body = notification.text;
  content.title = notification.advertiser;
  
  auto request = [UNNotificationRequest requestWithIdentifier:notification.id content:content trigger:trigger];
  const auto center = [UNUserNotificationCenter currentNotificationCenter];
  [center addNotificationRequest:request withCompletionHandler:^(NSError * _Nullable error) {
    if (error) {
      NSLog(@"Failed to add notification request");
    }
  }];
}

- (void)clearNotificationWithIdentifier:(NSString *)identifier
{
  const auto center = [UNUserNotificationCenter currentNotificationCenter];
  [center removePendingNotificationRequestsWithIdentifiers:@[identifier]];
  [center removeDeliveredNotificationsWithIdentifiers:@[identifier]];
}

#pragma mark - UNUserNotificationCenterDelegate

- (void)userNotificationCenter:(UNUserNotificationCenter *)center willPresentNotification:(UNNotification *)notification withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler
{
  if (![self shouldHandleNotificationRequest:notification.request]) {
    return;
  }
  
  const auto identifier = notification.request.identifier;
  [self.ads reportNotificationEvent:identifier eventType:BATAdsNotificationEventTypeViewed];
  
  // Show in the foreground
  completionHandler(UNNotificationPresentationOptionAlert);
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center didReceiveNotificationResponse:(UNNotificationResponse *)response withCompletionHandler:(void (^)())completionHandler
{
  if (![self shouldHandleNotificationRequest:response.notification.request]) {
    return;
  }
  
  const auto identifier = response.notification.request.identifier;
  
  auto eventType = BATAdsNotificationEventTypeClicked;
  if ([response.actionIdentifier isEqualToString:UNNotificationDismissActionIdentifier]) {
    eventType = BATAdsNotificationEventTypeDismissed;
  } else {
    const auto notification = [self.ads adsNotificationForIdentifier:identifier];
    // Not dismissed, and we don't register custom actions, so open the URL
    if (self.adTapped) {
      self.adTapped(notification);
    }
  }
  [self.ads reportNotificationEvent:identifier eventType:eventType];
  completionHandler();
}

@end
