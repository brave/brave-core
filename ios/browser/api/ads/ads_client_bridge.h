/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_BRIDGE_H_
#define BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_BRIDGE_H_

#import <Foundation/Foundation.h>

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#import "brave/components/brave_ads/core/public/ads_client/ads_client.h"

@protocol AdsClientBridge
@required

- (bool)isBrowserActive;
- (bool)isBrowserInFullScreenMode;
- (bool)canShowNotificationAdsWhileBrowserIsBackgrounded;
- (void)addObserver:(brave_ads::AdsClientNotifierObserver*)observer;
- (void)removeObserver:(brave_ads::AdsClientNotifierObserver*)observer;
- (void)notifyPendingObservers;
- (bool)isNetworkConnectionAvailable;
- (bool)canShowNotificationAds;
- (void)loadResourceComponent:(const std::string&)id
                      version:(int)version
                     callback:(brave_ads::LoadFileCallback)callback;
- (void)showScheduledCaptcha:(const std::string&)payment_id
                   captchaId:(const std::string&)captcha_id;
- (void)getSiteHistory:(int)max_count
               forDays:(int)days_ago
              callback:(brave_ads::GetSiteHistoryCallback)callback;
- (void)load:(const std::string&)name
    callback:(brave_ads::LoadCallback)callback;
- (std::string)loadDataResource:(const std::string&)name;
- (void)log:(const char*)file
            line:(int)line
    verboseLevel:(int)verbose_level
         message:(const std::string&)message;
- (void)save:(const std::string&)name
       value:(const std::string&)value
    callback:(brave_ads::SaveCallback)callback;
- (void)showNotificationAd:(const brave_ads::NotificationAdInfo&)info;
- (void)closeNotificationAd:(const std::string&)placement_id;
- (void)UrlRequest:(brave_ads::mojom::UrlRequestInfoPtr)url_request
          callback:(brave_ads::UrlRequestCallback)callback;
- (void)runDBTransaction:
            (brave_ads::mojom::DBTransactionInfoPtr)mojom_db_transaction
                callback:(brave_ads::RunDBTransactionCallback)callback;
- (void)setProfilePref:(const std::string&)path value:(base::Value)value;
- (bool)findProfilePref:(const std::string&)path;
- (std::optional<base::Value>)getProfilePref:(const std::string&)path;
- (void)clearProfilePref:(const std::string&)path;
- (bool)hasProfilePrefPath:(const std::string&)path;
- (void)setLocalStatePref:(const std::string&)path value:(base::Value)value;
- (bool)findLocalStatePref:(const std::string&)path;
- (std::optional<base::Value>)getLocalStatePref:(const std::string&)path;
- (void)clearLocalStatePref:(const std::string&)path;
- (bool)hasLocalStatePrefPath:(const std::string&)path;
- (base::Value::Dict)getVirtualPrefs;
- (void)recordP2AEvents:(const std::vector<std::string>&)events;

@end

#endif  // BRAVE_IOS_BROWSER_API_ADS_ADS_CLIENT_BRIDGE_H_
