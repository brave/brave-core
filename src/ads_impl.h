/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "ads_serve.h"
#include "bat/ads/ads.h"
#include "bat/ads/ad_info.h"
#include "bat/usermodel/user_model.h"
#include "event_type_blur_info.h"
#include "event_type_destroy_info.h"
#include "event_type_focus_info.h"
#include "event_type_load_info.h"
#include "bat/ads/event_type_notification_shown_info.h"
#include "bat/ads/event_type_sustain_info.h"
#include "bat/ads/notification_result_info.h"
#include "client.h"
#include "settings.h"
#include "bundle.h"

namespace ads {

class Settings;
class Client;
class Bundle;
class AdsServe;

class AdsImpl : public Ads, CallbackHandler {
 public:
  explicit AdsImpl(AdsClient* ads_client);
  ~AdsImpl() override;

  // Not copyable, not assignable
  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;

  void GenerateAdReportingNotificationShownEvent(
      const NotificationShownInfo& info) override;
  void GenerateAdReportingNotificationResultEvent(
      const NotificationResultInfo& info) override;
  void GenerateAdReportingSustainEvent(
      const SustainInfo& info) override;
  void Initialize() override;
  void InitializeUserModel(const std::string& json) override;
  void AppFocused(const bool is_focused) override;
  void TabUpdated(
      const std::string& tab_id,
      const std::string& url,
      const bool is_active,
      const bool is_incognito) override;
  void TabSwitched(
      const std::string& tab_id,
      const std::string& url,
      const bool is_incognito) override;
  void TabClosed(const std::string& tab_id) override;
  void RecordUnIdle() override;
  void RemoveAllHistory() override;
  void SaveCachedInfo() override;
  void RecordMediaPlaying(
      const std::string& tab_id,
      const bool is_playing) override;
  void ClassifyPage(const std::string& url, const std::string& html) override;
  void ChangeLocale(const std::string& locale) override;
  void CheckReadyAdServe(const bool forced = false) override;
  void ServeSampleAd() override;

  void SetNotificationsAvailable(const bool available) override;
  void SetNotificationsAllowed(const bool allowed) override;
  void SetNotificationsConfigured(const bool configured) override;
  void SetNotificationsExpired(const bool expired) override;

  void StartCollectingActivity(const uint64_t start_timer_in);

  void OnTimer(const uint32_t timer_id) override;

  void OnGetCategory(
      const Result result,
      const std::string& category,
      const std::vector<AdInfo>& ads) override;

  void OnGetSampleCategory(
      const Result result,
      const std::string& category) override;

 private:
  bool initialized_;
  bool IsInitialized();
  void Deinitialize();

  void OnUserModelLoaded(const Result result, const std::string& json);
  void OnSettingsLoaded(
      const Result result,
      const std::string& json);

  void OnClientSaved(const Result result);
  void OnClientLoaded(
      const Result result,
      const std::string& json);

  void OnBundleSaved(const Result result);
  void OnBundleLoaded(
      const Result result,
      const std::string& json);
  void OnBundleReset(const Result result);

  bool boot_;

  bool app_focused_;

  std::string last_page_classification_;
  void LoadUserModel();
  std::string GetWinningCategory(const std::vector<double>& page_score);
  std::string GetWinnerOverTimeCategory();

  std::map<std::string, std::vector<double>> page_score_cache_;
  void CachePageScore(
      const std::string& url,
      const std::vector<double>& page_score);

  uint32_t collect_activity_timer_id_;
  void CollectActivity();
  bool IsCollectingActivity() const;
  void StopCollectingActivity();

  void ConfirmAdUUIDIfAdEnabled();

  void RetrieveSSID();

  void TestShoppingData(const std::string& url);
  void TestSearchState(const std::string& url);

  std::map<std::string, bool> media_playing_;
  bool IsMediaPlaying() const;

  void ProcessLocales(const std::vector<std::string>& locales);

  void ServeAdFromCategory(
      const std::string& category);
  std::vector<AdInfo> GetUnseenAds(
      const std::vector<AdInfo>& ads);
  bool IsAllowedToShowAds();
  bool AdsShownHistoryRespectsRollingTimeConstraint(
      const std::deque<time_t> history,
      const uint64_t seconds_window,
      const uint64_t allowable_ad_count) const;

  uint64_t next_easter_egg_;
  void GenerateAdReportingLoadEvent(const LoadInfo info);
  void GenerateAdReportingBackgroundEvent();
  void GenerateAdReportingForegroundEvent();
  void GenerateAdReportingBlurEvent(const BlurInfo& info);
  void GenerateAdReportingDestroyEvent(const DestroyInfo& info);
  void GenerateAdReportingFocusEvent(const FocusInfo& info);
  void GenerateAdReportingRestartEvent();
  void GenerateAdReportingSettingsEvent();

  AdsClient* ads_client_;  // NOT OWNED

  std::unique_ptr<Settings> settings_;
  std::unique_ptr<Client> client_;
  std::unique_ptr<Bundle> bundle_;
  std::unique_ptr<AdsServe> ads_serve_;
  std::unique_ptr<usermodel::UserModel> user_model_;
};

}  // namespace ads
