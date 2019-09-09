/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ADS_IMPL_H_
#define BAT_ADS_INTERNAL_ADS_IMPL_H_

#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include <deque>
#include <memory>

#include "bat/ads/ads.h"
#include "bat/ads/ad_history_detail.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/notification_event_type.h"
#include "bat/ads/notification_info.h"

#include "bat/ads/internal/ads_serve.h"
#include "bat/ads/internal/bundle.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/internal/event_type_blur_info.h"
#include "bat/ads/internal/event_type_destroy_info.h"
#include "bat/ads/internal/event_type_focus_info.h"
#include "bat/ads/internal/event_type_load_info.h"
#include "bat/ads/internal/notification_result_type.h"
#include "bat/ads/internal/notifications.h"

#include "bat/usermodel/user_model.h"

namespace ads {

class Client;
class Bundle;
class AdsServe;
class Notifications;

class AdsImpl : public Ads {
 public:
  explicit AdsImpl(AdsClient* ads_client);
  ~AdsImpl() override;

  bool is_first_run_;

  InitializeCallback initialize_callback_;
  void Initialize(
      InitializeCallback callback) override;
  void InitializeStep2(
      const Result result);
  void InitializeStep3(
      const Result result);
  void InitializeStep4(
      const Result result);
  bool IsInitialized();

  void Shutdown(
      ShutdownCallback callback) override;

  void LoadUserModel();
  void OnUserModelLoaded(
      const Result result,
      const std::string& json);
  void InitializeUserModel(
      const std::string& json,
      const std::string& language);

  bool IsMobile() const;

  bool is_foreground_;
  void OnForeground() override;
  void OnBackground() override;
  bool IsForeground() const;

  void OnIdle() override;
  void OnUnIdle() override;

  std::map<int32_t, bool> media_playing_;
  void OnMediaPlaying(
      const int32_t tab_id) override;
  void OnMediaStopped(
      const int32_t tab_id) override;
  bool IsMediaPlaying() const;

  bool GetNotificationForId(
      const std::string& id,
      ads::NotificationInfo* notification) override;

  void OnNotificationEvent(
      const std::string& id,
      const ads::NotificationEventType type) override;
  void NotificationEventViewed(
      const std::string& id,
      const NotificationInfo& notification);
  void NotificationEventClicked(
      const std::string& id,
      const NotificationInfo& notification);
  void NotificationEventDismissed(
      const std::string& id,
      const NotificationInfo& notification);
  void NotificationEventTimedOut(
      const std::string& id,
      const NotificationInfo& notification);

  int32_t active_tab_id_;
  std::string active_tab_url_;
  std::string previous_tab_url_;
  void OnTabUpdated(
      const int32_t tab_id,
      const std::string& url,
      const bool is_active,
      const bool is_incognito) override;
  void OnTabClosed(
      const int32_t tab_id) override;

  void RemoveAllHistory(
      RemoveAllHistoryCallback callback) override;

  void SetConfirmationsIsReady(
      const bool is_ready) override;

  std::map<uint64_t, std::vector<AdsHistory>> GetAdsHistory() override;
  AdContent::LikeAction ToggleAdThumbUp(
      const std::string& id,
      const std::string& creative_set_id,
      const AdContent::LikeAction& action) override;
  AdContent::LikeAction ToggleAdThumbDown(
      const std::string& id,
      const std::string& creative_set_id,
      const AdContent::LikeAction& action) override;
  CategoryContent::OptAction ToggleAdOptInAction(
      const std::string& category,
      const CategoryContent::OptAction& action) override;
  CategoryContent::OptAction ToggleAdOptOutAction(
      const std::string& category,
      const CategoryContent::OptAction& action) override;
  bool ToggleSaveAd(
      const std::string& id,
      const std::string& creative_set_id,
      const bool saved) override;
  bool ToggleFlagAd(
      const std::string& id,
      const std::string& creative_set_id,
      const bool flagged) override;

  void ChangeLocale(
      const std::string& locale) override;

  void OnPageLoaded(
      const std::string& url,
      const std::string& html) override;

  void MaybeClassifyPage(
      const std::string& url,
      const std::string& html);
  bool ShouldClassifyPages() const;
  std::string ClassifyPage(
      const std::string& url,
      const std::string& html);

  std::string GetWinnerOverTimeCategory();
  std::string GetWinningCategory(
      const std::vector<double>& page_score);

  std::map<std::string, std::vector<double>> page_score_cache_;
  void CachePageScore(
      const std::string& url,
      const std::vector<double>& page_score);

  void TestShoppingData(
      const std::string& url);
  bool TestSearchState(
      const std::string& url);

  void ServeSampleAd() override;
  void OnLoadSampleBundle(
      const Result result,
      const std::string& json);

  void CheckEasterEgg(
      const std::string& url);

  void CheckReadyAdServe(
      const bool forced);
  void ServeAdFromCategory(
      const std::string& category);
  void OnServeAdFromCategory(
      const Result result,
      const std::string& category,
      const std::vector<AdInfo>& ads);
  bool ServeAdFromParentCategory(
      const std::string& category,
      const std::vector<AdInfo>& ads);
  void ServeUntargetedAd();
  void OnServeUntargetedAd(
      const Result result,
      const std::string& category,
      const std::vector<AdInfo>& ads);
  void ServeAd(
      const std::string& category,
      const std::vector<AdInfo>& ads);

  std::vector<AdInfo> GetEligibleAds(
      const std::vector<AdInfo>& ads);
  std::vector<AdInfo> GetUnseenAdsAndRoundRobinIfNeeded(
      const std::vector<AdInfo>& ads) const;
  std::vector<AdInfo> GetUnseenAds(
      const std::vector<AdInfo>& ads) const;

  bool AdRespectsTotalMaxFrequencyCapping(
      const AdInfo& ad);
  bool AdRespectsPerHourFrequencyCapping(
      const AdInfo& ad);
  bool AdRespectsPerDayFrequencyCapping(
      const AdInfo& ad);
  bool AdRespectsDailyCapFrequencyCapping(
      const AdInfo& ad);

  std::deque<uint64_t> GetAdsShownForId(
      const std::string& id);
  std::deque<uint64_t> GetCreativeSetForId(
      const std::string& id);
  std::deque<uint64_t> GetCampaignForId(
      const std::string& id);

  bool IsAdValid(
      const AdInfo& ad_info);
  NotificationInfo last_shown_notification_info_;
  bool ShowAd(
      const AdInfo& ad_info,
      const std::string& category);
  bool HistoryRespectsRollingTimeConstraint(
      const std::deque<uint64_t> history,
      const uint64_t seconds_window,
      const uint64_t allowable_ad_count) const;
  bool HistoryRespectsRollingTimeConstraint(
      const std::deque<AdHistoryDetail> history,
      const uint64_t seconds_window,
      const uint64_t allowable_ad_count) const;
  bool IsAllowedToServeAds();
  bool DoesHistoryRespectMinimumWaitTimeToServeAds();
  bool DoesHistoryRespectAdsPerDayLimit();

  uint32_t collect_activity_timer_id_;
  void StartCollectingActivity(
      const uint64_t start_timer_in);
  void CollectActivity();
  void StopCollectingActivity();
  bool IsCollectingActivity() const;

  uint32_t delivering_notifications_timer_id_;
  void StartDeliveringNotifications();
  void DeliverNotification();
  void StopDeliveringNotifications();
  bool IsDeliveringNotifications() const;
  bool IsCatalogOlderThanOneDay();
  void NotificationAllowedCheck(
      const bool serve);
	  
  #if defined(OS_ANDROID)
  void RemoveAllNotificationsAfterReboot();
  void RemoveAllNotificationsAfterUpdate();
  #endif

  void BundleUpdated();

  uint32_t sustained_ad_interaction_timer_id_;
  std::string last_sustaining_ad_url_;
  void StartSustainingAdInteraction(
      const uint64_t start_timer_in);
  void SustainAdInteractionIfNeeded();
  void SustainAdInteraction();
  void StopSustainingAdInteraction();
  bool IsSustainingAdInteraction() const;
  bool IsStillViewingAd() const;
  void ConfirmAd(
      const NotificationInfo& info,
      const ConfirmationType& type);
  void ConfirmAction(
      const std::string& uuid,
      const std::string& creative_set_id,
      const ConfirmationType& type);

  void OnTimer(
      const uint32_t timer_id) override;

  uint64_t next_easter_egg_timestamp_in_seconds_;
  void GenerateAdReportingConfirmationEvent(
      const NotificationInfo& info);
  void GenerateAdReportingConfirmationEvent(
      const std::string& uuid,
      const ConfirmationType& type);
  void MaybeGenerateAdReportingLoadEvent(
      const std::string& url,
      const std::string& classification);
  void GenerateAdReportingLoadEvent(
      const LoadInfo& info);
  void GenerateAdReportingBackgroundEvent();
  void GenerateAdReportingForegroundEvent();
  void GenerateAdReportingBlurEvent(
      const BlurInfo& info);
  void GenerateAdReportingDestroyEvent(
      const DestroyInfo& info);
  void GenerateAdReportingFocusEvent(
      const FocusInfo& info);
  void GenerateAdReportingRestartEvent();
  void GenerateAdReportingSettingsEvent();
  void GenerateAdReportingNotificationShownEvent(
      const NotificationInfo& info);
  void GenerateAdReportingNotificationResultEvent(
      const NotificationInfo& info,
      const NotificationResultInfoResultType type);

  void GenerateAdsHistoryEntry(
      const NotificationInfo& notification_info,
      const ConfirmationType& type);

  bool IsNotificationFromSampleCatalog(
      const NotificationInfo& info) const;
  bool IsCreativeSetFromSampleCatalog(
      const std::string& creative_set_id) const;

  bool IsSupportedUrl(
      const std::string& url) const;
  bool UrlHostsMatch(
      const std::string& url_1,
      const std::string& url_2) const;

  std::unique_ptr<Client> client_;
  std::unique_ptr<Bundle> bundle_;
  std::unique_ptr<AdsServe> ads_serve_;
  std::unique_ptr<Notifications> notifications_;
  std::unique_ptr<usermodel::UserModel> user_model_;

 private:
  bool is_initialized_;

  bool is_confirmations_ready_;

  AdsClient* ads_client_;  // NOT OWNED

  // Not copyable, not assignable
  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_ADS_IMPL_H_
