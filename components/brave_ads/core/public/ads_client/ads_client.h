/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_H_

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"
#include "brave/components/brave_ads/core/public/export.h"

namespace brave_ads {

class AdsClientNotifierObserver;
struct NotificationAdInfo;

class ADS_EXPORT AdsClient {
 public:
  virtual ~AdsClient() = default;

  // Called to add an ads client observer. Observers will not be scheduled until
  // `NotifyPendingObservers()` is called.
  virtual void AddObserver(AdsClientNotifierObserver* observer) = 0;

  // Called to remove an ads client observer.
  virtual void RemoveObserver(AdsClientNotifierObserver* observer) = 0;

  // Called to bind pending ads client observers.
  virtual void NotifyPendingObservers() = 0;

  // Returns `true` if there is an available network connection.
  virtual bool IsNetworkConnectionAvailable() const = 0;

  // Returns `true` if the browser is active.
  virtual bool IsBrowserActive() const = 0;

  // Returns `true` if the browser is in full screen mode.
  virtual bool IsBrowserInFullScreenMode() const = 0;

  // Returns `true` if notification ads can be shown.
  virtual bool CanShowNotificationAds() = 0;

  // Returns `true` if notification ads can be shown while the browser is
  // backgrounded.
  virtual bool CanShowNotificationAdsWhileBrowserIsBackgrounded() const = 0;

  // Show notification `ad`.
  virtual void ShowNotificationAd(const NotificationAdInfo& ad) = 0;

  // Close the notification ad for the specified `placement_id`.
  virtual void CloseNotificationAd(const std::string& placement_id) = 0;

  // Cache an ad event for the specified instance `id`, `ad_type`,
  // `confirmation_type` and `time`.
  virtual void CacheAdEventForInstanceId(const std::string& id,
                                         const std::string& ad_type,
                                         const std::string& confirmation_type,
                                         base::Time time) const = 0;

  // Get cached ad events for the specified `ad_type` and `confirmation_type`.
  virtual std::vector<base::Time> GetCachedAdEvents(
      const std::string& ad_type,
      const std::string& confirmation_type) const = 0;

  // Reset ad event cache for the specified instance `id`.
  virtual void ResetAdEventCacheForInstanceId(const std::string& id) const = 0;

  // Get site history from `recent_day_range` limited to `max_count` items. The
  // callback takes one argument - `SiteHistoryList` containing a list of URLs.
  virtual void GetSiteHistory(int max_count,
                              int recent_day_range,
                              GetSiteHistoryCallback callback) = 0;

  // Fetch and return data for the `url_request`. Loading should be performed
  // asynchronously, so that the app remains responsive and should handle
  // incoming data or errors as they arrive. The callback takes one argument -
  // `URLResponse` containing the URL response.
  virtual void UrlRequest(mojom::UrlRequestInfoPtr url_request,
                          UrlRequestCallback callback) = 0;

  // Save a value for the specified `name` to persistent storage. The callback
  // takes one argument - `bool` is set to `true` if successful otherwise
  // `false`.
  virtual void Save(const std::string& name,
                    const std::string& value,
                    SaveCallback callback) = 0;

  // Load a file for the specified `name` from persistent storage. The callback
  // takes one argument - optional containing the loaded `value`.
  virtual void Load(const std::string& name, LoadCallback callback) = 0;

  // Load a resource component for the specified `id` and `version` from
  // persistent storage. The callback takes one argument - `base::File` will be
  // valid if successful otherwise invalid.
  virtual void LoadResourceComponent(const std::string& id,
                                     int version,
                                     LoadFileCallback callback) = 0;

  // Load a data resource for the specified `name`. Returns the resource if
  // successful otherwise an empty string.
  virtual std::string LoadDataResource(const std::string& name) = 0;

  // Show a notification indicating that a scheduled captcha with the given
  // `captcha_id` must be solved for the given `payment_id` before the user can
  // continue to be served ads.
  virtual void ShowScheduledCaptcha(const std::string& payment_id,
                                    const std::string& captcha_id) = 0;

  // Run a database transaction. The callback takes one argument -
  // `mojom::DBCommandResponseInfoPtr` containing the info of the transaction.
  virtual void RunDBTransaction(mojom::DBTransactionInfoPtr transaction,
                                RunDBTransactionCallback callback) = 0;

  // Record P2A (Private Advertising Analytics) `events`.
  virtual void RecordP2AEvents(const std::vector<std::string>& events) = 0;

  // Get the value from the specified profile preference `path`. Returns the
  // default value if the path does not exist.
  virtual std::optional<base::Value> GetProfilePref(
      const std::string& path) = 0;

  // Update the value for the specified profile preference `path`.
  virtual void SetProfilePref(const std::string& path, base::Value value) = 0;

  // Remove the preference from the specified profile `path`.
  virtual void ClearProfilePref(const std::string& path) = 0;

  // Returns `true` if a value has been set for the specified profile preference
  // `path`.
  virtual bool HasProfilePrefPath(const std::string& path) const = 0;

  // Get the value from the specified local state preference `path`. Returns the
  // default value if the path does not exist.
  virtual std::optional<base::Value> GetLocalStatePref(
      const std::string& path) = 0;

  // Update the value for the specified local state preference `path`.
  virtual void SetLocalStatePref(const std::string& path,
                                 base::Value value) = 0;

  // Remove the preference from the specified local state `path`.
  virtual void ClearLocalStatePref(const std::string& path) = 0;

  // Returns `true` if a value has been set for the specified local state
  // preference `path`.
  virtual bool HasLocalStatePrefPath(const std::string& path) const = 0;

  // Log a `message` to `file` and the console log with `line` and
  // `verbose_level`.
  virtual void Log(const char* file,
                   int line,
                   int verbose_level,
                   const std::string& message) = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_CLIENT_ADS_CLIENT_H_
