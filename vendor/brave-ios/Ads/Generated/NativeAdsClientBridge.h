/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "bat/ads/ads_client.h"

@protocol NativeAdsClientBridge
@required

- (uint64_t)getAdsPerHour;
- (bool)isAdsEnabled;
- (bool)shouldAllowAdConversionTracking;
- (bool)isForeground;
- (bool)isFullScreen;
- (bool)canShowBackgroundNotifications;
- (bool)isNetworkConnectionAvailable;
- (bool)shouldShowNotifications;
- (void)loadAdsResource:(const std::string&)id
                version:(const int)version
               callback:(ads::LoadCallback)callback;
- (void)getBrowsingHistory:(const int)max_count
                   forDays:(const int)days_ago
                  callback:(ads::GetBrowsingHistoryCallback)callback;
- (void)load:(const std::string &)name callback:(ads::LoadCallback)callback;
- (std::string)loadResourceForId:(const std::string &)id;
- (void)log:(const char *)file line:(const int)line verboseLevel:(const int)verbose_level message:(const std::string &) message;
- (void)save:(const std::string &)name value:(const std::string &)value callback:(ads::ResultCallback)callback;
- (void)setIdleThreshold:(const int)threshold;
- (void)showNotification:(const ads::AdNotificationInfo &)info;
- (void)closeNotification:(const std::string&)id;
- (void)recordAdEvent:(const std::string&)ad_type
     confirmationType:(const std::string&)confirmation_type
            timestamp:(const uint64_t)timestamp;
- (std::vector<uint64_t>)getAdEvents:(const std::string&)ad_type
                    confirmationType:(const std::string&)confirmation_type;
- (void)UrlRequest:(ads::UrlRequestPtr)url_request callback:(ads::UrlRequestCallback)callback;
- (bool)shouldAllowAdsSubdivisionTargeting;
- (void)setAllowAdsSubdivisionTargeting:(const bool)should_allow;
- (std::string)adsSubdivisionTargetingCode;
- (void)setAdsSubdivisionTargetingCode:(const std::string &)subdivision_targeting_code;
- (std::string)autoDetectedAdsSubdivisionTargetingCode;
- (void)setAutoDetectedAdsSubdivisionTargetingCode:(const std::string &)subdivision_targeting_code;
- (void)runDBTransaction:(ads::DBTransactionPtr)transaction callback:(ads::RunDBTransactionCallback)callback;
- (void)onAdRewardsChanged;
- (void)setBooleanPref:(const std::string&)path value:(const bool)value;
- (bool)getBooleanPref:(const std::string&)path;
- (void)setIntegerPref:(const std::string&)path value:(const int)value;
- (int)getIntegerPref:(const std::string&)path;
- (void)setDoublePref:(const std::string&)path value:(const double)value;
- (double)getDoublePref:(const std::string&)path;
- (void)setStringPref:(const std::string&)path value:(const std::string&)value;
- (std::string)getStringPref:(const std::string&)path;
- (void)setInt64Pref:(const std::string&)path value:(const int64_t)value;
- (int64_t)getInt64Pref:(const std::string&)path;
- (void)setUint64Pref:(const std::string&)path value:(const uint64_t)value;
- (uint64_t)getUint64Pref:(const std::string&)path;
- (void)clearPref:(const std::string&)path;
- (void)recordP2AEvent:(const std::string&)name type:(const ads::P2AEventType)type value:(const std::string&)value;

@end
