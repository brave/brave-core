/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_UTIL_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace brave_ads {

class AdsClientNotifierObserver;
struct NotificationAdInfo;

void AddAdsClientNotifierObserver(AdsClientNotifierObserver* observer);
void RemoveAdsClientNotifierObserver(AdsClientNotifierObserver* observer);
void NotifyPendingAdsClientObservers();

bool IsNetworkConnectionAvailable();

bool IsBrowserActive();
bool IsBrowserInFullScreenMode();

bool CanShowNotificationAds();
bool CanShowNotificationAdsWhileBrowserIsBackgrounded();
void ShowNotificationAd(const NotificationAdInfo& ad);
void CloseNotificationAd(const std::string& placement_id);

void CacheAdEventForInstanceId(const std::string& id,
                               AdType ad_type,
                               ConfirmationType confirmation_type,
                               base::Time time);
std::vector<base::Time> GetCachedAdEvents(AdType ad_type,
                                          ConfirmationType confirmation_type);
void ResetAdEventCacheForInstanceId(const std::string& id);

void GetSiteHistory(int max_count,
                    int recent_day_range,
                    GetSiteHistoryCallback callback);

void UrlRequest(mojom::UrlRequestInfoPtr url_request,
                UrlRequestCallback callback);

void Save(const std::string& name,
          const std::string& value,
          SaveCallback callback);
void Load(const std::string& name, LoadCallback callback);

void LoadResourceComponent(const std::string& id,
                           int version,
                           LoadFileCallback callback);

std::string LoadDataResource(const std::string& name);

void ShowScheduledCaptcha(const std::string& payment_id,
                          const std::string& captcha_id);

void RunDBTransaction(mojom::DBTransactionInfoPtr transaction,
                      RunDBTransactionCallback callback);

void RecordP2AEvents(const std::vector<std::string>& events);

std::optional<base::Value> GetProfilePref(const std::string& path);
bool GetProfileBooleanPref(const std::string& path);
int GetProfileIntegerPref(const std::string& path);
double GetProfileDoublePref(const std::string& path);
std::string GetProfileStringPref(const std::string& path);
base::Value::Dict GetProfileDictPref(const std::string& path);
base::Value::List GetProfileListPref(const std::string& path);
int64_t GetProfileInt64Pref(const std::string& path);
uint64_t GetProfileUint64Pref(const std::string& path);
base::Time GetProfileTimePref(const std::string& path);
base::TimeDelta GetProfileTimeDeltaPref(const std::string& path);
void SetProfilePref(const std::string& path, base::Value value);
void SetProfileBooleanPref(const std::string& path, bool value);
void SetProfileIntegerPref(const std::string& path, int value);
void SetProfileDoublePref(const std::string& path, double value);
void SetProfileStringPref(const std::string& path, const std::string& value);
void SetProfileDictPref(const std::string& path, base::Value::Dict value);
void SetProfileListPref(const std::string& path, base::Value::List value);
void SetProfileInt64Pref(const std::string& path, int64_t value);
void SetProfileUint64Pref(const std::string& path, uint64_t value);
void SetProfileTimePref(const std::string& path, base::Time value);
void SetProfileTimeDeltaPref(const std::string& path, base::TimeDelta value);
void ClearProfilePref(const std::string& path);
bool HasProfilePrefPath(const std::string& path);

std::optional<base::Value> GetLocalStatePref(const std::string& path);
bool GetLocalStateBooleanPref(const std::string& path);
int GetLocalStateIntegerPref(const std::string& path);
double GetLocalStateDoublePref(const std::string& path);
std::string GetLocalStateStringPref(const std::string& path);
base::Value::Dict GetLocalStateDictPref(const std::string& path);
base::Value::List GetLocalStateListPref(const std::string& path);
int64_t GetLocalStateInt64Pref(const std::string& path);
uint64_t GetLocalStateUint64Pref(const std::string& path);
base::Time GetLocalStateTimePref(const std::string& path);
base::TimeDelta GetLocalStateTimeDeltaPref(const std::string& path);
void SetLocalStatePref(const std::string& path, base::Value value);
void SetLocalStateBooleanPref(const std::string& path, bool value);
void SetLocalStateIntegerPref(const std::string& path, int value);
void SetLocalStateDoublePref(const std::string& path, double value);
void SetLocalStateStringPref(const std::string& path, const std::string& value);
void SetLocalStateDictPref(const std::string& path, base::Value::Dict value);
void SetLocalStateListPref(const std::string& path, base::Value::List value);
void SetLocalStateInt64Pref(const std::string& path, int64_t value);
void SetLocalStateUint64Pref(const std::string& path, uint64_t value);
void SetLocalStateTimePref(const std::string& path, base::Time value);
void SetLocalStateTimeDeltaPref(const std::string& path, base::TimeDelta value);
void ClearLocalStatePref(const std::string& path);
bool HasLocalStatePrefPath(const std::string& path);

void Log(const char* file,
         int line,
         int verbose_level,
         const std::string& message);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_UTIL_H_
