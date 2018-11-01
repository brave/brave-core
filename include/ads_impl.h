/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "bat-native-usermodel/include/user_model.h"

#include "ads.h"
#include "catalog_ads_serve.h"
#include "bundle_category_info.h"
#include "event_type_blur_info.h"
#include "event_type_destroy_info.h"
#include "event_type_focus_info.h"
#include "event_type_load_info.h"
#include "event_type_notification_shown_info.h"
#include "event_type_notification_result_info.h"
#include "event_type_sustain_info.h"
#include "client.h"
#include "settings.h"
#include "bundle.h"

namespace state {
class Settings;
class Client;
class Bundle;
}  // namespace state

namespace catalog {
class AdsServe;
}  // namespace catalog

namespace rewards_ads {

class AdsImpl : public ads::Ads, ads::CallbackHandler {
 public:
  explicit AdsImpl(ads::AdsClient* ads_client);
  ~AdsImpl() override;

  // Not copyable, not assignable
  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;

  void GenerateAdReportingNotificationShownEvent(
      const event_type::NotificationShownInfo& info) override;
  void GenerateAdReportingNotificationResultEvent(
      const event_type::NotificationResultInfo& info) override;
  void GenerateAdReportingSustainEvent(
      const event_type::SustainInfo& info) override;
  void Initialize() override;
  void InitializeUserModel(const std::string& json) override;
  void AppFocused(const bool focused) override;
  void TabUpdated(
      const std::string& tab_id,
      const std::string& url,
      const bool active,
      const bool incognito) override;
  void TabSwitched(
      const std::string& tab_id,
      const std::string& url,
      const bool incognito) override;
  void TabClosed(const std::string& tab_id) override;
  void RecordUnIdle() override;
  void RemoveAllHistory() override;
  void SaveCachedInfo() override;
  void RecordMediaPlaying(
      const std::string& tab_id,
      const bool active) override;
  void ClassifyPage(const std::string& url, const std::string& html) override;
  void ChangeLocale(const std::string& locale) override;
  void CollectActivity() override;
  void CheckReadyAdServe(const bool forced = false) override;
  void ServeSampleAd() override;

  void SetNotificationsAvailable(const bool available) override;
  void SetNotificationsAllowed(const bool allowed) override;
  void SetNotificationsConfigured(const bool configured) override;
  void SetNotificationsExpired(const bool expired) override;

  void StartCollectingActivity(const uint64_t start_timer_in);

  void OnTimer(const uint32_t timer_id) override;

  void OnUserModelLoaded(const ads::Result result) override;

  void OnSettingsLoaded(
      const ads::Result result,
      const std::string& json) override;

  void OnClientSaved(const ads::Result result) override;
  void OnClientLoaded(
      const ads::Result result,
      const std::string& json) override;

  void OnBundleSaved(const ads::Result result) override;
  void OnBundleLoaded(
      const ads::Result result,
      const std::string& json) override;

  void OnGetSampleCategory(
      const ads::Result result,
      const std::string& category) override;

  void OnGetAds(
      const ads::Result result,
      const std::string& category,
      const std::vector<bundle::CategoryInfo>& ads) override;

 private:
  bool initialized_;
  bool IsInitialized();
  void Deinitialize();

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
  std::vector<bundle::CategoryInfo> GetUnseenAds(
      const std::vector<bundle::CategoryInfo>& categories);
  bool IsAllowedToShowAds();
  bool AdsShownHistoryRespectsRollingTimeConstraint(
      const std::deque<time_t> history,
      const uint64_t seconds_window,
      const uint64_t allowable_ad_count) const;

  uint64_t next_easter_egg_;
  void GenerateAdReportingLoadEvent(const event_type::LoadInfo info);
  void GenerateAdReportingBackgroundEvent();
  void GenerateAdReportingForegroundEvent();
  void GenerateAdReportingBlurEvent(const event_type::BlurInfo& info);
  void GenerateAdReportingDestroyEvent(const event_type::DestroyInfo& info);
  void GenerateAdReportingFocusEvent(const event_type::FocusInfo& info);
  void GenerateAdReportingRestartEvent();
  void GenerateAdReportingSettingsEvent();

  ads::AdsClient* ads_client_;  // NOT OWNED

  std::unique_ptr<state::Settings> settings_;
  std::unique_ptr<state::Client> client_;
  std::shared_ptr<state::Bundle> bundle_;
  std::unique_ptr<catalog::AdsServe> catalog_ads_serve_;
  std::unique_ptr<usermodel::UserModel> user_model_;
};

}  // namespace rewards_ads
