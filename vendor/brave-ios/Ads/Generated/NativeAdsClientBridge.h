/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "bat/ads/ads_client.h"

@protocol NativeAdsClientBridge
@required

- (void)confirmAd:(std::unique_ptr<ads::NotificationInfo>)info;
- (void)eventLog:(const std::string &)json;
- (void)getAds:(const std::string &)category callback:(ads::OnGetAdsCallback)callback;
- (const std::string)getAdsLocale;
- (uint64_t)getAdsPerDay;
- (uint64_t)getAdsPerHour;
- (void)getClientInfo:(ads::ClientInfo *)info;
- (const std::vector<std::string>)getLocales;
- (bool)isAdsEnabled;
- (bool)isForeground;
- (bool)isNetworkConnectionAvailable;
- (bool)isNotificationsAvailable;
- (void)killTimer:(uint32_t)timer_id;
- (void)load:(const std::string &)name callback:(ads::OnLoadCallback)callback;
- (const std::string)loadJsonSchema:(const std::string &)name;
- (void)loadSampleBundle:(ads::OnLoadSampleBundleCallback)callback;
- (void)loadUserModelForLocale:(const std::string &)locale callback:(ads::OnLoadCallback)callback;
- (std::unique_ptr<ads::LogStream>)log:(const char *)file line:(const int)line logLevel:(const ads::LogLevel)log_level;
- (void)reset:(const std::string &)name callback:(ads::OnResetCallback)callback;
- (void)save:(const std::string &)name value:(const std::string &)value callback:(ads::OnSaveCallback)callback;
- (void)saveBundleState:(std::unique_ptr<ads::BundleState>)state callback:(ads::OnSaveCallback)callback;
- (void)setCatalogIssuers:(std::unique_ptr<ads::IssuersInfo>)info;
- (void)setIdleThreshold:(const int)threshold;
- (uint32_t)setTimer:(const uint64_t)time_offset;
- (void)showNotification:(std::unique_ptr<ads::NotificationInfo>)info;
- (void)closeNotification:(const std::string&)id;
- (void)URLRequest:(const std::string &)url headers:(const std::vector<std::string> &)headers content:(const std::string &)content contentType:(const std::string &)content_type method:(const ads::URLRequestMethod)method callback:(ads::URLRequestCallback)callback;

@end
