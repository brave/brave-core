/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/browser/ads_service_observer.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sessions/core/session_id.h"
#include "url/gurl.h"

namespace ads {
struct AdsHistoryInfo;
}

namespace base {
class DictionaryValue;
class ListValue;
}

namespace user_prefs {
class PrefRegistrySyncable;
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

using OnGetInlineContentAdCallback = base::OnceCallback<
    void(const bool, const std::string&, const base::DictionaryValue&)>;

using GetAccountStatementCallback = base::OnceCallback<void(const bool,
                                                            const double,
                                                            const int64_t,
                                                            const int,
                                                            const double,
                                                            const double)>;

class AdsService : public KeyedService {
 public:
  AdsService();
  ~AdsService() override;

  AdsService(const AdsService&) = delete;
  AdsService& operator=(const AdsService&) = delete;

  void AddObserver(AdsServiceObserver* observer);
  void RemoveObserver(AdsServiceObserver* observer);

  virtual bool IsSupportedLocale() const = 0;
  virtual bool IsNewlySupportedLocale() = 0;

  virtual bool IsEnabled() const = 0;
  virtual void SetEnabled(const bool is_enabled) = 0;

  virtual void SetAllowConversionTracking(const bool should_allow) = 0;

  virtual int64_t GetAdsPerHour() const = 0;
  virtual void SetAdsPerHour(const int64_t ads_per_hour) = 0;

  virtual bool ShouldAllowAdsSubdivisionTargeting() const = 0;
  virtual std::string GetAdsSubdivisionTargetingCode() const = 0;
  virtual void SetAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) = 0;
  virtual std::string GetAutoDetectedAdsSubdivisionTargetingCode() const = 0;
  virtual void SetAutoDetectedAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) = 0;

  virtual void OnShowAdNotification(const std::string& notification_id) = 0;
  virtual void OnCloseAdNotification(const std::string& notification_id,
                                     const bool by_user) = 0;
  virtual void OnClickAdNotification(const std::string& notification_id) = 0;

  virtual void ChangeLocale(const std::string& locale) = 0;

  virtual void OnHtmlLoaded(const SessionID& tab_id,
                            const std::vector<GURL>& redirect_chain,
                            const std::string& html) = 0;

  virtual void OnTextLoaded(const SessionID& tab_id,
                            const std::vector<GURL>& redirect_chain,
                            const std::string& text) = 0;

  virtual void OnUserGesture(const int32_t page_transition_type) = 0;

  virtual void OnMediaStart(const SessionID& tab_id) = 0;
  virtual void OnMediaStop(const SessionID& tab_id) = 0;

  virtual void OnTabUpdated(const SessionID& tab_id,
                            const GURL& url,
                            const bool is_active,
                            const bool is_browser_active) = 0;

  virtual void OnTabClosed(const SessionID& tab_id) = 0;

  virtual void OnResourceComponentUpdated(const std::string& id) = 0;

  virtual void OnNewTabPageAdEvent(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const ads::mojom::BraveAdsNewTabPageAdEventType event_type) = 0;

  virtual void OnPromotedContentAdEvent(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const ads::mojom::BraveAdsPromotedContentAdEventType event_type) = 0;

  virtual void GetInlineContentAd(const std::string& dimensions,
                                  OnGetInlineContentAdCallback callback) = 0;

  virtual void OnInlineContentAdEvent(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const ads::mojom::BraveAdsInlineContentAdEventType event_type) = 0;

  virtual void ReconcileAdRewards() = 0;

  virtual void GetAdsHistory(const uint64_t from_timestamp,
                             const uint64_t to_timestamp,
                             OnGetAdsHistoryCallback callback) = 0;

  virtual void GetAccountStatement(GetAccountStatementCallback callback) = 0;

  virtual void ToggleAdThumbUp(const std::string& creative_instance_id,
                               const std::string& creative_set_id,
                               const int action,
                               OnToggleAdThumbUpCallback callback) = 0;
  virtual void ToggleAdThumbDown(const std::string& creative_instance_id,
                                 const std::string& creative_set_id,
                                 const int action,
                                 OnToggleAdThumbDownCallback callback) = 0;
  virtual void ToggleAdOptInAction(const std::string& category,
                                   const int action,
                                   OnToggleAdOptInActionCallback callback) = 0;
  virtual void ToggleAdOptOutAction(
      const std::string& category,
      const int action,
      OnToggleAdOptOutActionCallback callback) = 0;
  virtual void ToggleSaveAd(const std::string& creative_instance_id,
                            const std::string& creative_set_id,
                            const bool saved,
                            OnToggleSaveAdCallback callback) = 0;
  virtual void ToggleFlagAd(const std::string& creative_instance_id,
                            const std::string& creative_set_id,
                            const bool flagged,
                            OnToggleFlagAdCallback callback) = 0;

  virtual void ResetAllState(const bool should_shutdown) = 0;

  // static
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

 protected:
  base::ObserverList<AdsServiceObserver> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
