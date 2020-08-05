/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "bat/ads/ads_client.h"

@protocol NativeAdsClientBridge
@required

- (uint64_t)getAdsPerDay;
- (uint64_t)getAdsPerHour;
- (bool)isAdsEnabled;
- (bool)shouldAllowAdConversionTracking;
- (bool)isForeground;
- (bool)canShowBackgroundNotifications;
- (bool)isNetworkConnectionAvailable;
- (bool)shouldShowNotifications;
- (void)loadUserModelForId:(const std::string &)id callback:(ads::LoadCallback)callback;
- (void)load:(const std::string &)name callback:(ads::LoadCallback)callback;
- (std::string)loadResourceForId:(const std::string &)id;
- (void)log:(const char *)file line:(const int)line verboseLevel:(const int)verbose_level message:(const std::string &) message;
- (void)save:(const std::string &)name value:(const std::string &)value callback:(ads::ResultCallback)callback;
- (void)setIdleThreshold:(const int)threshold;
- (void)showNotification:(std::unique_ptr<ads::AdNotificationInfo>)info;
- (void)closeNotification:(const std::string&)id;
- (void)UrlRequest:(ads::UrlRequestPtr)url_request callback:(ads::UrlRequestCallback)callback;
- (bool)shouldAllowAdsSubdivisionTargeting;
- (void)setAllowAdsSubdivisionTargeting:(const bool)should_allow;
- (std::string)adsSubdivisionTargetingCode;
- (void)setAdsSubdivisionTargetingCode:(const std::string &)subdivision_targeting_code;
- (std::string)automaticallyDetectedAdsSubdivisionTargetingCode;
- (void)setAutomaticallyDetectedAdsSubdivisionTargetingCode:(const std::string &)subdivision_targeting_code;
- (void)runDBTransaction:(ads::DBTransactionPtr)transaction callback:(ads::RunDBTransactionCallback)callback;
- (void)onAdRewardsChanged;

@end
