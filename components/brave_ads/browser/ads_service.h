/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_

#include <map>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "build/build_config.h"
#include "brave/components/brave_ads/browser/ads_service_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_id.h"
#include "url/gurl.h"

namespace ads {
struct AdsHistory;
}

namespace base {
class ListValue;
}

namespace brave_ads {

using OnGetAdsHistoryCallback =
    base::OnceCallback<void(const base::ListValue&)>;

using OnToggleAdThumbUpCallback =
    base::OnceCallback<void(const std::string&, int)>;
using OnToggleAdThumbDownCallback =
    base::OnceCallback<void(const std::string&, int)>;
using OnToggleAdOptInActionCallback =
    base::OnceCallback<void(const std::string&, int)>;
using OnToggleAdOptOutActionCallback =
    base::OnceCallback<void(const std::string&, int)>;
using OnToggleSaveAdCallback =
    base::OnceCallback<void(const std::string&, bool)>;
using OnToggleFlagAdCallback =
    base::OnceCallback<void(const std::string&, bool)>;

using GetTransactionHistoryCallback = base::OnceCallback<void(
    const bool, const double, const uint64_t, const uint64_t)>;

class AdsService : public KeyedService {
 public:
  AdsService();
  ~AdsService() override;

  AdsService(const AdsService&) = delete;
  AdsService& operator=(const AdsService&) = delete;

  virtual bool IsSupportedLocale() const = 0;
  virtual bool IsNewlySupportedLocale() = 0;

  virtual bool IsEnabled() const = 0;
  virtual void SetEnabled(
      const bool is_enabled) = 0;

  virtual bool ShouldAllowAdConversionTracking() const = 0;
  virtual void SetAllowAdConversionTracking(
      const bool should_allow) = 0;

  virtual uint64_t GetAdsPerHour() = 0;
  virtual void SetAdsPerHour(
      const uint64_t ads_per_hour) = 0;

  virtual bool ShouldAllowAdsSubdivisionTargeting() const = 0;
  virtual void SetAllowAdsSubdivisionTargeting(
      const bool should_allow) = 0;

  virtual std::string GetAdsSubdivisionTargetingCode() const = 0;
  virtual void SetAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) = 0;

  virtual std::string
  GetAutomaticallyDetectedAdsSubdivisionTargetingCode() const = 0;
  virtual void SetAutomaticallyDetectedAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) = 0;

  virtual void ChangeLocale(
      const std::string& locale) = 0;

  virtual void OnPageLoaded(
      const SessionID& tab_id,
      const GURL& original_url,
      const GURL& url,
      const std::string& html) = 0;

  virtual void OnMediaStart(
      const SessionID& tab_id) = 0;
  virtual void OnMediaStop(
      const SessionID& tab_id) = 0;

  virtual void OnTabUpdated(
      const SessionID& tab_id,
      const GURL& url,
      const bool is_active,
      const bool is_browser_active) = 0;
  virtual void OnTabClosed(
      const SessionID& tab_id) = 0;

  virtual void UpdateAdRewards(
      const bool should_reconcile) = 0;

  virtual void GetAdsHistory(
      const uint64_t from_timestamp,
      const uint64_t to_timestamp,
      OnGetAdsHistoryCallback callback) = 0;

  virtual void GetTransactionHistory(
      GetTransactionHistoryCallback callback) = 0;

  virtual void ToggleAdThumbUp(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const int action,
      OnToggleAdThumbUpCallback callback) = 0;
  virtual void ToggleAdThumbDown(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const int action,
      OnToggleAdThumbDownCallback callback) = 0;
  virtual void ToggleAdOptInAction(
      const std::string& category,
      const int action,
      OnToggleAdOptInActionCallback callback) = 0;
  virtual void ToggleAdOptOutAction(
      const std::string& category,
      const int action,
      OnToggleAdOptOutActionCallback callback) = 0;
  virtual void ToggleSaveAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool saved,
      OnToggleSaveAdCallback callback) = 0;
  virtual void ToggleFlagAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool flagged,
      OnToggleFlagAdCallback callback) = 0;

  virtual void ResetAllState(
      const bool should_shutdown) = 0;

  virtual void OnUserModelUpdated(
      const std::string& id) = 0;

  void AddObserver(
      AdsServiceObserver* observer);
  void RemoveObserver(
      AdsServiceObserver* observer);

 protected:
  base::ObserverList<AdsServiceObserver> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
