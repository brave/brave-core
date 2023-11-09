/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"

#include <utility>

#include "base/check.h"
#include "base/json/values_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/client/ads_client.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"  // IWYU pragma: keep

namespace brave_ads {

namespace {

bool HasInstance() {
  return GlobalState::HasInstance();
}

AdsClient* GetInstance() {
  CHECK(HasInstance());

  AdsClient* ads_client = GlobalState::GetInstance()->GetAdsClient();
  CHECK(ads_client);

  return ads_client;
}

}  // namespace

void AddAdsClientNotifierObserver(AdsClientNotifierObserver* observer) {
  GetInstance()->AddObserver(observer);
}

void RemoveAdsClientNotifierObserver(AdsClientNotifierObserver* observer) {
  GetInstance()->RemoveObserver(observer);
}

void NotifyPendingAdsClientObservers() {
  GetInstance()->NotifyPendingObservers();
}

bool IsNetworkConnectionAvailable() {
  return GetInstance()->IsNetworkConnectionAvailable();
}

bool IsBrowserActive() {
  return GetInstance()->IsBrowserActive();
}

bool IsBrowserInFullScreenMode() {
  return GetInstance()->IsBrowserInFullScreenMode();
}

bool CanShowNotificationAds() {
  return GetInstance()->CanShowNotificationAds();
}

bool CanShowNotificationAdsWhileBrowserIsBackgrounded() {
  return GetInstance()->CanShowNotificationAdsWhileBrowserIsBackgrounded();
}

void ShowNotificationAd(const NotificationAdInfo& ad) {
  GetInstance()->ShowNotificationAd(ad);
}

void CloseNotificationAd(const std::string& placement_id) {
  GetInstance()->CloseNotificationAd(placement_id);
}

void ShowReminder(mojom::ReminderType type) {
  GetInstance()->ShowReminder(type);
}

void CacheAdEventForInstanceId(const std::string& id,
                               const std::string& ad_type,
                               const std::string& confirmation_type,
                               base::Time time) {
  GetInstance()->CacheAdEventForInstanceId(id, ad_type, confirmation_type,
                                           time);
}

std::vector<base::Time> GetCachedAdEvents(
    const std::string& ad_type,
    const std::string& confirmation_type) {
  return GetInstance()->GetCachedAdEvents(ad_type, confirmation_type);
}

void ResetAdEventCacheForInstanceId(const std::string& id) {
  GetInstance()->ResetAdEventCacheForInstanceId(id);
}

void GetBrowsingHistory(int max_count,
                        int recent_day_range,
                        GetBrowsingHistoryCallback callback) {
  GetInstance()->GetBrowsingHistory(max_count, recent_day_range,
                                    std::move(callback));
}

void UrlRequest(mojom::UrlRequestInfoPtr url_request,
                UrlRequestCallback callback) {
  GetInstance()->UrlRequest(std::move(url_request), std::move(callback));
}

void Save(const std::string& name,
          const std::string& value,
          SaveCallback callback) {
  GetInstance()->Save(name, value, std::move(callback));
}

void Load(const std::string& name, LoadCallback callback) {
  GetInstance()->Load(name, std::move(callback));
}

void LoadFileResource(const std::string& id,
                      int version,
                      LoadFileCallback callback) {
  GetInstance()->LoadFileResource(id, version, std::move(callback));
}

std::string LoadDataResource(const std::string& name) {
  return GetInstance()->LoadDataResource(name);
}

void GetScheduledCaptcha(const std::string& payment_id,
                         GetScheduledCaptchaCallback callback) {
  GetInstance()->GetScheduledCaptcha(payment_id, std::move(callback));
}

void ShowScheduledCaptchaNotification(const std::string& payment_id,
                                      const std::string& captcha_id) {
  GetInstance()->ShowScheduledCaptchaNotification(payment_id, captcha_id);
}

void RunDBTransaction(mojom::DBTransactionInfoPtr transaction,
                      RunDBTransactionCallback callback) {
  GetInstance()->RunDBTransaction(std::move(transaction), std::move(callback));
}

void UpdateAdRewards() {
  GetInstance()->UpdateAdRewards();
}

void RecordP2AEvents(const std::vector<std::string>& events) {
  GetInstance()->RecordP2AEvents(events);
}

void AddFederatedLearningPredictorTrainingSample(
    std::vector<brave_federated::mojom::CovariateInfoPtr> training_sample) {
  GetInstance()->AddFederatedLearningPredictorTrainingSample(
      std::move(training_sample));
}

absl::optional<base::Value> GetProfilePref(const std::string& path) {
  return GetInstance()->GetProfilePref(path);
}

bool GetProfileBooleanPref(const std::string& path) {
  const absl::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return false;
  }

  CHECK(value->is_bool()) << "Wrong type for GetProfileBooleanPref: " << path;

  return value->GetBool();
}

int GetProfileIntegerPref(const std::string& path) {
  const absl::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_int()) << "Wrong type for GetProfileIntegerPref: " << path;

  return value->GetInt();
}

double GetProfileDoublePref(const std::string& path) {
  const absl::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return 0.0;
  }

  CHECK(value->is_double()) << "Wrong type for GetProfileDoublePref: " << path;

  return value->GetDouble();
}

std::string GetProfileStringPref(const std::string& path) {
  const absl::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return "";
  }

  CHECK(value->is_string()) << "Wrong type for GetProfileStringPref: " << path;

  return value->GetString();
}

base::Value::Dict GetProfileDictPref(const std::string& path) {
  const absl::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_dict()) << "Wrong type for GetProfileDictPref: " << path;

  return value->GetDict().Clone();
}

base::Value::List GetProfileListPref(const std::string& path) {
  const absl::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_list()) << "Wrong type for GetProfileListPref: " << path;

  return value->GetList().Clone();
}

int64_t GetProfileInt64Pref(const std::string& path) {
  const absl::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_string()) << "Wrong type for GetProfileInt64Pref: " << path;

  const absl::optional<int64_t> integer = base::ValueToInt64(*value);
  return integer.value_or(0);
}

uint64_t GetProfileUint64Pref(const std::string& path) {
  const absl::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_string()) << "Wrong type for GetProfileUint64Pref: " << path;

  uint64_t integer;
  const bool success = base::StringToUint64(value->GetString(), &integer);
  DCHECK(success) << "GetProfileUint64Pref failed: " << path;
  if (!success) {
    return 0;
  }

  return integer;
}

base::Time GetProfileTimePref(const std::string& path) {
  const absl::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_string()) << "Wrong type for GetProfileTimePref: " << path;

  const absl::optional<base::Time> time = base::ValueToTime(*value);
  return time.value_or(base::Time());
}

base::TimeDelta GetProfileTimeDeltaPref(const std::string& path) {
  const absl::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_string())
      << "Wrong type for GetProfileTimedDeltaPref: " << path;

  const absl::optional<base::TimeDelta> time_delta =
      base::ValueToTimeDelta(*value);
  return time_delta.value_or(base::TimeDelta());
}

void SetProfilePref(const std::string& path, base::Value value) {
  GetInstance()->SetProfilePref(path, std::move(value));
}

void SetProfileBooleanPref(const std::string& path, const bool value) {
  SetProfilePref(path, base::Value(value));
}

void SetProfileIntegerPref(const std::string& path, const int value) {
  SetProfilePref(path, base::Value(value));
}

void SetProfileDoublePref(const std::string& path, const double value) {
  SetProfilePref(path, base::Value(value));
}

void SetProfileStringPref(const std::string& path, const std::string& value) {
  SetProfilePref(path, base::Value(value));
}

void SetProfileDictPref(const std::string& path, base::Value::Dict value) {
  SetProfilePref(path, base::Value(std::move(value)));
}

void SetProfileListPref(const std::string& path, base::Value::List value) {
  SetProfilePref(path, base::Value(std::move(value)));
}

void SetProfileInt64Pref(const std::string& path, const int64_t value) {
  SetProfilePref(path, base::Int64ToValue(value));
}

void SetProfileUint64Pref(const std::string& path, const uint64_t value) {
  SetProfilePref(path, base::Value(base::NumberToString(value)));
}

void SetProfileTimePref(const std::string& path, base::Time value) {
  SetProfilePref(path, base::TimeToValue(value));
}

void SetProfileTimeDeltaPref(const std::string& path, base::TimeDelta value) {
  SetProfilePref(path, base::TimeDeltaToValue(value));
}

void ClearProfilePref(const std::string& path) {
  GetInstance()->ClearProfilePref(path);
}

bool HasProfilePrefPath(const std::string& path) {
  return GetInstance()->HasProfilePrefPath(path);
}

absl::optional<base::Value> GetLocalStatePref(const std::string& path) {
  return GetInstance()->GetLocalStatePref(path);
}

bool GetLocalStateBooleanPref(const std::string& path) {
  const absl::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return false;
  }

  CHECK(value->is_bool()) << "Wrong type for GetProfileBooleanPref: " << path;

  return value->GetBool();
}

int GetLocalStateIntegerPref(const std::string& path) {
  const absl::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_int()) << "Wrong type for GetProfileBooleanPref: " << path;

  return value->GetInt();
}

double GetLocalStateDoublePref(const std::string& path) {
  const absl::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return 0.0;
  }

  CHECK(value->is_double()) << "Wrong type for GetProfileBooleanPref: " << path;

  return value->GetDouble();
}

std::string GetLocalStateStringPref(const std::string& path) {
  const absl::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return "";
  }

  CHECK(value->is_string()) << "Wrong type for GetProfileBooleanPref: " << path;

  return value->GetString();
}

base::Value::Dict GetLocalStateDictPref(const std::string& path) {
  const absl::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_dict()) << "Wrong type for GetLocalStateDictPref: " << path;

  return value->GetDict().Clone();
}

base::Value::List GetLocalStateListPref(const std::string& path) {
  const absl::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_list()) << "Wrong type for GetLocalStateListPref: " << path;

  return value->GetList().Clone();
}

int64_t GetLocalStateInt64Pref(const std::string& path) {
  const absl::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_string()) << "Wrong type for GetProfileBooleanPref: " << path;

  const absl::optional<int64_t> integer = base::ValueToInt64(*value);
  return integer.value_or(0);
}

uint64_t GetLocalStateUint64Pref(const std::string& path) {
  const absl::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_string()) << "Wrong type for GetProfileBooleanPref: " << path;

  uint64_t integer;
  const bool success = base::StringToUint64(value->GetString(), &integer);
  DCHECK(success) << "GetLocalStateUint64Pref failed: " << path;
  if (!success) {
    return 0;
  }

  return integer;
}

base::Time GetLocalStateTimePref(const std::string& path) {
  const absl::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_string()) << "Wrong type for GetLocalStateTimePref: " << path;

  const absl::optional<base::Time> time = base::ValueToTime(*value);
  return time.value_or(base::Time());
}

base::TimeDelta GetLocalStateTimeDeltaPref(const std::string& path) {
  const absl::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_string())
      << "Wrong type for GetLocalStateTimedDeltaPref: " << path;

  const absl::optional<base::TimeDelta> time_delta =
      base::ValueToTimeDelta(*value);
  return time_delta.value_or(base::TimeDelta());
}

void SetLocalStatePref(const std::string& path, base::Value value) {
  GetInstance()->SetLocalStatePref(path, std::move(value));
}

void SetLocalStateBooleanPref(const std::string& path, const bool value) {
  SetLocalStatePref(path, base::Value(value));
}

void SetLocalStateIntegerPref(const std::string& path, const int value) {
  SetLocalStatePref(path, base::Value(value));
}

void SetLocalStateDoublePref(const std::string& path, const double value) {
  SetLocalStatePref(path, base::Value(value));
}

void SetLocalStateStringPref(const std::string& path,
                             const std::string& value) {
  SetLocalStatePref(path, base::Value(value));
}

void SetLocalStateDictPref(const std::string& path, base::Value::Dict value) {
  SetLocalStatePref(path, base::Value(std::move(value)));
}

void SetLocalStateListPref(const std::string& path, base::Value::List value) {
  SetLocalStatePref(path, base::Value(std::move(value)));
}

void SetLocalStateInt64Pref(const std::string& path, const int64_t value) {
  SetLocalStatePref(path, base::Int64ToValue(value));
}

void SetLocalStateUint64Pref(const std::string& path, const uint64_t value) {
  SetLocalStatePref(path, base::Value(base::NumberToString(value)));
}

void SetLocalStateTimePref(const std::string& path, base::Time value) {
  SetLocalStatePref(path, base::TimeToValue(value));
}

void SetLocalStateTimeDeltaPref(const std::string& path,
                                base::TimeDelta value) {
  SetLocalStatePref(path, base::TimeDeltaToValue(value));
}

void ClearLocalStatePref(const std::string& path) {
  GetInstance()->ClearLocalStatePref(path);
}

bool HasLocalStatePrefPath(const std::string& path) {
  return GetInstance()->HasLocalStatePrefPath(path);
}

void Log(const char* file,
         int line,
         int verbose_level,
         const std::string& message) {
  if (HasInstance()) {
    GetInstance()->Log(file, line, verbose_level, message);
  }
}

}  // namespace brave_ads
