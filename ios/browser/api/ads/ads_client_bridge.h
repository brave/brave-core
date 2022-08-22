/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_BRIDGE_H_
#define BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_BRIDGE_H_

#import <Foundation/Foundation.h>
#import "bat/ads/ads_client.h"

#include <string>
#include <vector>

@protocol AdsClientBridge
@required

- (bool)isBrowserActive;
- (bool)isBrowserInFullScreenMode;
- (bool)canShowNotificationAdsWhileBrowserIsBackgrounded;
- (bool)isNetworkConnectionAvailable;
- (bool)canShowNotificationAds;
- (void)loadFileResource:(const std::string&)id
                 version:(const int)version
                callback:(ads::LoadFileCallback)callback;
- (void)clearScheduledCaptcha;
- (void)getScheduledCaptcha:(const std::string&)payment_id
                   callback:(ads::GetScheduledCaptchaCallback)callback;
- (void)showScheduledCaptchaNotification:(const std::string&)payment_id
                               captchaId:(const std::string&)captcha_id;
- (void)getBrowsingHistory:(const int)max_count
                   forDays:(const int)days_ago
                  callback:(ads::GetBrowsingHistoryCallback)callback;
- (void)load:(const std::string&)name callback:(ads::LoadCallback)callback;
- (std::string)loadDataResource:(const std::string&)name;
- (void)log:(const char*)file
            line:(const int)line
    verboseLevel:(const int)verbose_level
         message:(const std::string&)message;
- (void)save:(const std::string&)name
       value:(const std::string&)value
    callback:(ads::SaveCallback)callback;
- (void)showNotificationAd:(const ads::NotificationAdInfo&)info;
- (void)closeNotificationAd:(const std::string&)placement_id;
- (void)recordAdEventForId:(const std::string&)id
                    adType:(const std::string&)ad_type
          confirmationType:(const std::string&)confirmation_type
                      time:(const base::Time)time;
- (std::vector<base::Time>)getAdEventHistory:(const std::string&)ad_type
                            confirmationType:
                                (const std::string&)confirmation_type;
- (void)resetAdEventHistoryForId:(const std::string&)id;
- (void)UrlRequest:(ads::mojom::UrlRequestInfoPtr)url_request
          callback:(ads::UrlRequestCallback)callback;
- (void)runDBTransaction:(ads::mojom::DBTransactionInfoPtr)transaction
                callback:(ads::RunDBTransactionCallback)callback;
- (void)updateAdRewards;
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
- (void)setTimePref:(const std::string&)path value:(const base::Time)value;
- (base::Time)getTimePref:(const std::string&)path;
- (void)setDictPref:(const std::string&)path value:(base::Value::Dict)value;
- (absl::optional<base::Value::Dict>)getDictPref:(const std::string&)path;
- (void)setListPref:(const std::string&)path value:(base::Value::List)value;
- (absl::optional<base::Value::List>)getListPref:(const std::string&)path;
- (void)clearPref:(const std::string&)path;
- (bool)hasPrefPath:(const std::string&)path;
- (void)recordP2AEvent:(const std::string&)name value:(base::Value::List)value;
- (void)logTrainingInstance:
    (const std::vector<brave_federated::mojom::CovariateInfoPtr>)
        training_instance;

@end

#endif  // BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_BRIDGE_H_
