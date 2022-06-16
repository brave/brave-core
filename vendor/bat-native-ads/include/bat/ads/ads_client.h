/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_H_

#include <cstdint>
#include <string>
#include <vector>

#include "bat/ads/ads_client_aliases.h"
#include "bat/ads/export.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

struct NotificationAdInfo;

class ADS_EXPORT AdsClient {
 public:
  virtual ~AdsClient() = default;

  // Returns |true| if there is an available network connection.
  virtual bool IsNetworkConnectionAvailable() const = 0;

  // Returns |true| if the browser is active.
  virtual bool IsBrowserActive() const = 0;

  // Returns |true| if the browser is in full screen mode.
  virtual bool IsBrowserInFullScreenMode() const = 0;

  // Returns |true| if notifications should be displayed.
  virtual bool ShouldShowNotifications() = 0;

  // Returns |true| if notifications can be displayed while the browser is
  // backgrounded.
  virtual bool CanShowBackgroundNotifications() const = 0;

  // Display |notification_ad| on the screen.
  virtual void ShowNotification(const NotificationAdInfo& notification_ad) = 0;

  // Close the notification for the specified |uuid|.
  virtual void CloseNotification(const std::string& uuid) = 0;

  // Record an ad event for the specified |id|, |ad_type|, |confirmation_type|
  // and |time|.
  virtual void RecordAdEventForId(const std::string& id,
                                  const std::string& ad_type,
                                  const std::string& confirmation_type,
                                  const base::Time time) const = 0;

  // Get ad event history for the specified |ad_type| and |confirmation_type|.
  virtual std::vector<base::Time> GetAdEvents(
      const std::string& ad_type,
      const std::string& confirmation_type) const = 0;

  // Reset ad events for the specified |id|.
  virtual void ResetAdEventsForId(const std::string& id) const = 0;

  // Get browsing history from |days_ago| limited to |max_count| items. The
  // callback takes one argument - |std::vector<GURL>| containing a list of
  // URLs.
  virtual void GetBrowsingHistory(const int max_count,
                                  const int days_ago,
                                  GetBrowsingHistoryCallback callback) = 0;

  // Fetch and return data for the |url_request|. Loading should be performed
  // asynchronously, so that the app remains responsive and should handle
  // incoming data or errors as they arrive. The callback takes 1 argument -
  // |URLResponse| containing the URL response.
  virtual void UrlRequest(mojom::UrlRequestPtr url_request,
                          UrlRequestCallback callback) = 0;

  // Save a value for the specified |name| to persistent storage. The callback
  // takes one argument - |bool| is set to |true| if successful otherwise
  // |false|.
  virtual void Save(const std::string& name,
                    const std::string& value,
                    ResultCallback callback) = 0;

  // Load a file for the specified |name| from persistent storage. The callback
  // takes 2 arguments - |bool| is set to |true| if successful otherwise
  // |false|. |value| containing the persisted value.
  virtual void Load(const std::string& name, LoadCallback callback) = 0;

  // Load a file resource for the specified |id| and |version| from persistent
  // storage. The callback takes 1 argument - |base::File| will be valid if
  // successful otherwise invalid.
  virtual void LoadFileResource(const std::string& id,
                                const int version,
                                LoadFileCallback callback) = 0;

  // Load a data resource for the specified |name|. Returns the resource if
  // successful otherwise an empty string.
  virtual std::string LoadDataResource(const std::string& name) = 0;

  // Clear the currently scheduled captcha, if any.
  virtual void ClearScheduledCaptcha() = 0;

  // Retrieves the captcha scheduled for the specified |payment_id|, if any. The
  // callback takes 1 argument - |std::string| containing a captcha id if the
  // user must solve a captcha otherwise the an empty string.
  virtual void GetScheduledCaptcha(const std::string& payment_id,
                                   GetScheduledCaptchaCallback callback) = 0;

  // Display a notification indicating that a scheduled captcha with the given
  // |captcha_id| must be solved for the given |payment_id| before the user can
  // continue to see ads.
  virtual void ShowScheduledCaptchaNotification(
      const std::string& payment_id,
      const std::string& captcha_id) = 0;

  // Run a database transaction. The callback takes one argument -
  // |mojom::DBCommandResponsePtr| containing the info of the transaction.
  virtual void RunDBTransaction(mojom::DBTransactionPtr transaction,
                                RunDBTransactionCallback callback) = 0;

  // Called to notify that ad rewards have changed.
  virtual void OnAdRewardsChanged() = 0;

  // Record a P2A (Privacy Preserving Anonymous) event with |value| for the
  // specfied |name| and |type|.
  virtual void RecordP2AEvent(const std::string& name,
                              const mojom::P2AEventType type,
                              const std::string& value) = 0;

  // Log |training_instance|.
  virtual void LogTrainingInstance(
      const brave_federated::mojom::TrainingInstancePtr training_instance) = 0;

  // Log a |message| to |file| and the console log with |line| and
  // |verbose_level|.
  virtual void Log(const char* file,
                   const int line,
                   const int verbose_level,
                   const std::string& message) = 0;

  // Get the value from the specified preference |path|. Returns a default value
  // if the path does not exist.
  virtual bool GetBooleanPref(const std::string& path) const = 0;
  virtual int GetIntegerPref(const std::string& path) const = 0;
  virtual double GetDoublePref(const std::string& path) const = 0;
  virtual std::string GetStringPref(const std::string& path) const = 0;
  virtual int64_t GetInt64Pref(const std::string& path) const = 0;
  virtual uint64_t GetUint64Pref(const std::string& path) const = 0;
  virtual base::Time GetTimePref(const std::string& path) const = 0;

  // Update the value for the specified preference |path|.
  virtual void SetBooleanPref(const std::string& path, const bool value) = 0;
  virtual void SetIntegerPref(const std::string& path, const int value) = 0;
  virtual void SetDoublePref(const std::string& path, const double value) = 0;
  virtual void SetStringPref(const std::string& path,
                             const std::string& value) = 0;
  virtual void SetInt64Pref(const std::string& path, const int64_t value) = 0;
  virtual void SetUint64Pref(const std::string& path, const uint64_t value) = 0;
  virtual void SetTimePref(const std::string& path, const base::Time value) = 0;

  // Remove the preference from the specified |path|.
  virtual void ClearPref(const std::string& path) = 0;

  // Returns |true| if a value has been set for the specified preference |path|.
  virtual bool HasPrefPath(const std::string& path) const = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_CLIENT_H_
