/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_CLIENT_ADS_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_CLIENT_ADS_CLIENT_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/client/ads_client_callback.h"
#include "brave/components/brave_ads/core/public/export.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom-forward.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

class AdsClientNotifierObserver;
struct NotificationAdInfo;

class ADS_EXPORT AdsClient {
 public:
  virtual ~AdsClient() = default;

  // Called to add an ads client observer. Observers will not be scheduled until
  // |BindObservers| is called.
  virtual void AddObserver(AdsClientNotifierObserver* observer) = 0;

  // Called to remove an ads client observer.
  virtual void RemoveObserver(AdsClientNotifierObserver* observer) = 0;

  // Called to bind pending ads client observers.
  virtual void NotifyPendingObservers() = 0;

  // Returns |true| if there is an available network connection.
  virtual bool IsNetworkConnectionAvailable() const = 0;

  // Returns |true| if the browser is active.
  virtual bool IsBrowserActive() const = 0;

  // Returns |true| if the browser is in full screen mode.
  virtual bool IsBrowserInFullScreenMode() const = 0;

  // Returns |true| if notification ads can be shown.
  virtual bool CanShowNotificationAds() = 0;

  // Returns |true| if notification ads can be shown while the browser is
  // backgrounded.
  virtual bool CanShowNotificationAdsWhileBrowserIsBackgrounded() const = 0;

  // Show notification |ad|.
  virtual void ShowNotificationAd(const NotificationAdInfo& ad) = 0;

  // Close the notification ad for the specified |placement_id|.
  virtual void CloseNotificationAd(const std::string& placement_id) = 0;

  // Show reminder for the specified |type|.
  virtual void ShowReminder(mojom::ReminderType type) = 0;

  // Cache an ad event for the specified instance |id|, |ad_type|,
  // |confirmation_type| and |time|.
  virtual void CacheAdEventForInstanceId(const std::string& id,
                                         const std::string& ad_type,
                                         const std::string& confirmation_type,
                                         base::Time time) const = 0;

  // Get cached ad events for the specified |ad_type| and |confirmation_type|.
  virtual std::vector<base::Time> GetCachedAdEvents(
      const std::string& ad_type,
      const std::string& confirmation_type) const = 0;

  // Reset ad event cache for the specified instance |id|.
  virtual void ResetAdEventCacheForInstanceId(const std::string& id) const = 0;

  // Get browsing history from |recent_day_range| limited to |max_count| items.
  // The callback takes one argument - |std::vector<GURL>| containing a list of
  // URLs.
  virtual void GetBrowsingHistory(int max_count,
                                  int recent_day_range,
                                  GetBrowsingHistoryCallback callback) = 0;

  // Fetch and return data for the |url_request|. Loading should be performed
  // asynchronously, so that the app remains responsive and should handle
  // incoming data or errors as they arrive. The callback takes one argument -
  // |URLResponse| containing the URL response.
  virtual void UrlRequest(mojom::UrlRequestInfoPtr url_request,
                          UrlRequestCallback callback) = 0;

  // Save a value for the specified |name| to persistent storage. The callback
  // takes one argument - |bool| is set to |true| if successful otherwise
  // |false|.
  virtual void Save(const std::string& name,
                    const std::string& value,
                    SaveCallback callback) = 0;

  // Load a file for the specified |name| from persistent storage. The callback
  // takes one argument - optional containing the loaded |value|.
  virtual void Load(const std::string& name, LoadCallback callback) = 0;

  // Load a file resource for the specified |id| and |version| from persistent
  // storage. The callback takes one argument - |base::File| will be valid if
  // successful otherwise invalid.
  virtual void LoadFileResource(const std::string& id,
                                int version,
                                LoadFileCallback callback) = 0;

  // Load a data resource for the specified |name|. Returns the resource if
  // successful otherwise an empty string.
  virtual std::string LoadDataResource(const std::string& name) = 0;

  // Retrieves the captcha scheduled for the specified |payment_id|, if any. The
  // callback takes one argument - |std::string| containing a captcha id if the
  // user must solve a captcha otherwise an empty string.
  virtual void GetScheduledCaptcha(const std::string& payment_id,
                                   GetScheduledCaptchaCallback callback) = 0;

  // Show a notification indicating that a scheduled captcha with the given
  // |captcha_id| must be solved for the given |payment_id| before the user can
  // continue to be served ads.
  virtual void ShowScheduledCaptchaNotification(
      const std::string& payment_id,
      const std::string& captcha_id) = 0;

  // Run a database transaction. The callback takes one argument -
  // |mojom::DBCommandResponseInfoPtr| containing the info of the transaction.
  virtual void RunDBTransaction(mojom::DBTransactionInfoPtr transaction,
                                RunDBTransactionCallback callback) = 0;

  // Called to update brave://rewards.
  virtual void UpdateAdRewards() = 0;

  // Record P2A (Private Advertising Analytics) |events|.
  virtual void RecordP2AEvents(const std::vector<std::string>& events) = 0;

  // Add federated learning |training_sample|.
  virtual void AddFederatedLearningPredictorTrainingSample(
      std::vector<brave_federated::mojom::CovariateInfoPtr>
          training_sample) = 0;

  // Get the value from the specified preference |path|. Returns the default
  // value if the path does not exist.
  virtual bool GetBooleanPref(const std::string& path) const = 0;
  virtual int GetIntegerPref(const std::string& path) const = 0;
  virtual double GetDoublePref(const std::string& path) const = 0;
  virtual std::string GetStringPref(const std::string& path) const = 0;
  virtual int64_t GetInt64Pref(const std::string& path) const = 0;
  virtual uint64_t GetUint64Pref(const std::string& path) const = 0;
  virtual base::Time GetTimePref(const std::string& path) const = 0;
  virtual absl::optional<base::Value::Dict> GetDictPref(
      const std::string& path) const = 0;
  virtual absl::optional<base::Value::List> GetListPref(
      const std::string& path) const = 0;

  // Update the value for the specified preference |path|.
  virtual void SetBooleanPref(const std::string& path, bool value) = 0;
  virtual void SetIntegerPref(const std::string& path, int value) = 0;
  virtual void SetDoublePref(const std::string& path, double value) = 0;
  virtual void SetStringPref(const std::string& path,
                             const std::string& value) = 0;
  virtual void SetInt64Pref(const std::string& path, int64_t value) = 0;
  virtual void SetUint64Pref(const std::string& path, uint64_t value) = 0;
  virtual void SetTimePref(const std::string& path, base::Time value) = 0;
  virtual void SetDictPref(const std::string& path,
                           base::Value::Dict value) = 0;
  virtual void SetListPref(const std::string& path,
                           base::Value::List value) = 0;

  // Remove the preference from the specified |path|.
  virtual void ClearPref(const std::string& path) = 0;

  // Returns |true| if a value has been set for the specified preference |path|.
  virtual bool HasPrefPath(const std::string& path) const = 0;

  // Get the value from the specified local state preference |path|. Returns the
  // default value if the path does not exist.
  virtual absl::optional<base::Value> GetLocalStatePref(
      const std::string& path) const = 0;

  // Update the value for the specified local state preference |path|.
  virtual void SetLocalStatePref(const std::string& path,
                                 base::Value value) = 0;

  // Log a |message| to |file| and the console log with |line| and
  // |verbose_level|.
  virtual void Log(const char* file,
                   int line,
                   int verbose_level,
                   const std::string& message) = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_CLIENT_ADS_CLIENT_H_
