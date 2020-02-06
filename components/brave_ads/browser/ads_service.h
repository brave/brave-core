/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_ads/browser/publisher_ads.h"
#include "base/macros.h"
#include "build/build_config.h"
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

using OnGetPublisherAdsCallback = base::OnceCallback<void(const std::string&,
    const std::vector<std::string>&, const base::ListValue&)>;

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

class AdsService : public KeyedService {
 public:
  AdsService() = default;

  virtual bool IsSupportedLocale() const = 0;
  virtual bool IsNewlySupportedLocale() = 0;

  virtual bool IsEnabled() const = 0;
  virtual void SetEnabled(
      const bool is_enabled) = 0;

  virtual bool ShouldShowPublisherAdsOnPariticipatingSites() const = 0;
  virtual void SetShowPublisherAdsOnPariticipatingSites(
      const bool should_show) = 0;

  virtual bool ShouldAllowAdConversionTracking() const = 0;
  virtual void SetAllowAdConversionTracking(
      const bool should_allow) = 0;

  virtual uint64_t GetAdsPerHour() const = 0;
  virtual void SetAdsPerHour(
      const uint64_t ads_per_hour) = 0;

  virtual void SetConfirmationsIsReady(
      const bool is_ready) = 0;

  virtual void ChangeLocale(
      const std::string& locale) = 0;

  virtual void OnPageLoaded(
      const std::string& url,
      const std::string& html) = 0;

  virtual void OnMediaStart(
      const SessionID& tab_id) = 0;
  virtual void OnMediaStop(
      const SessionID& tab_id) = 0;

  virtual void OnTabUpdated(
      const SessionID& tab_id,
      const GURL& url,
      const bool is_active) = 0;
  virtual void OnTabClosed(
      const SessionID& tab_id) = 0;

  virtual void OnPublisherAdEvent(
      const PublisherAdInfo& info,
      const PublisherAdEventType event_type) = 0;

  virtual void GetAdsHistory(
      const uint64_t from_timestamp,
      const uint64_t to_timestamp,
      OnGetAdsHistoryCallback callback) = 0;

  virtual void GetPublisherAds(
      const std::string& url,
      const std::vector<std::string>& sizes,
      OnGetPublisherAdsCallback callback) = 0;

  virtual void ToggleAdThumbUp(
      const std::string& id,
      const std::string& creative_set_id,
      const int action,
      OnToggleAdThumbUpCallback callback) = 0;
  virtual void ToggleAdThumbDown(
      const std::string& id,
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
      const std::string& id,
      const std::string& creative_set_id,
      const bool saved,
      OnToggleSaveAdCallback callback) = 0;
  virtual void ToggleFlagAd(
      const std::string& id,
      const std::string& creative_set_id,
      const bool flagged,
      OnToggleFlagAdCallback callback) = 0;

  virtual void ResetTheWholeState(
      const base::Callback<void(bool)>& callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AdsService);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_H_
