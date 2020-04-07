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
#include "bat/ads/ads_history.h"
#include "bat/ads/creative_ad_notification_info.h"
#include "bat/ads/mojom.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/ads_serve.h"
#include "bat/ads/internal/bundle.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/internal/ad_conversions.h"
#include "bat/ads/internal/ad_notification_result_type.h"
#include "bat/ads/internal/ad_notifications.h"
#include "bat/usermodel/user_model.h"
#include "bat/ads/internal/purchase_intent/purchase_intent_classifier.h"

namespace ads {

using WinningCategoryList = std::vector<std::string>;
using CategoryList = std::vector<std::string>;

class Client;
class Bundle;
class AdsServe;
class AdNotifications;
class AdConversions;
class FrequencyCapping;
class ExclusionRule;
class PermissionRule;
class PurchaseIntentClassifier;
struct PurchaseIntentSignalInfo;

class AdsImpl : public Ads {
 public:
  explicit AdsImpl(
      AdsClient* ads_client);
  ~AdsImpl() override;

  AdsClient* get_ads_client() const;
  Client* get_client() const;

  AdNotifications* get_ad_notifications() const;

  InitializeCallback initialize_callback_;
  void Initialize(
      InitializeCallback callback) override;
  void InitializeStep2(
      const Result result);
  void InitializeStep3(
      const Result result);
  void InitializeStep4(
      const Result result);
  void InitializeStep5(
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
  bool IsAndroid() const;

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

  bool GetAdNotification(
      const std::string& uuid,
      AdNotificationInfo* info) override;
  void OnAdNotificationEvent(
      const std::string& uuid,
      const AdNotificationEventType event_type) override;

  bool ShouldNotDisturb() const;

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

  AdsHistory GetAdsHistory(
      const AdsHistory::FilterType filter_type,
      const AdsHistory::SortType sort_type,
      const uint64_t from_timestamp,
      const uint64_t to_timestamp) override;

  AdContent::LikeAction ToggleAdThumbUp(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContent::LikeAction& action) override;
  AdContent::LikeAction ToggleAdThumbDown(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const AdContent::LikeAction& action) override;
  CategoryContent::OptAction ToggleAdOptInAction(
      const std::string& category,
      const CategoryContent::OptAction& action) override;
  CategoryContent::OptAction ToggleAdOptOutAction(
      const std::string& category,
      const CategoryContent::OptAction& action) override;
  bool ToggleSaveAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool saved) override;
  bool ToggleFlagAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool flagged) override;

  void ChangeLocale(
      const std::string& locale) override;

  void OnPageLoaded(
      const std::string& url,
      const std::string& content) override;

  void ExtractPurchaseIntentSignal(
      const std::string& url);
  void GeneratePurchaseIntentSignalHistoryEntry(
      const PurchaseIntentSignalInfo& purchase_intent_signal);

  void MaybeClassifyPage(
      const std::string& url,
      const std::string& content);
  bool ShouldClassifyPagesIfTargeted() const;
  std::string ClassifyPage(
      const std::string& url,
      const std::string& content);

  WinningCategoryList GetWinningCategories();
  PurchaseIntentWinningCategoryList GetWinningPurchaseIntentCategories();
  std::string GetWinningCategory(
      const std::vector<double>& page_score);

  std::map<std::string, std::vector<double>> page_score_cache_;
  void CachePageScore(
      const std::string& url,
      const std::vector<double>& page_score);
  const std::map<std::string, std::vector<double>>& GetPageScoreCache() const;

  void ServeSampleAd() override;
  void OnLoadSampleBundle(
      const Result result,
      const std::string& json);

  void CheckEasterEgg(
      const std::string& url);

  void MaybeServeAdNotification(
      const bool should_serve);
  void ServeAdNotificationIfReady(
      const bool should_force);
  void ServeAdNotificationFromCategories(
      const std::vector<std::string>& categories);
  void OnServeAdNotificationFromCategories(
      const Result result,
      const std::vector<std::string>& categories,
      const CreativeAdNotificationList& ads);
  bool ServeAdNotificationFromParentCategories(
      const std::vector<std::string>& categories);
  void ServeUntargetedAdNotification();
  void OnServeUntargetedAdNotification(
      const Result result,
      const std::vector<std::string>& categories,
      const CreativeAdNotificationList& ads);
  CategoryList GetCategoriesToServeAd();
  void ServeAdNotification(
      const CreativeAdNotificationList& ads);
  void OnServeAdNotification(
      const Result result,
      const AdConversionList& ad_conversions,
      const CreativeAdNotificationList& ads);
  void SuccessfullyServedAd();
  void FailedToServeAdNotification(
      const std::string& reason);

  CreativeAdNotificationList GetEligibleAds(
      const CreativeAdNotificationList& ads);
  CreativeAdNotificationList GetEligibleAdsForConversions(
      const CreativeAdNotificationList& ads,
      const AdConversionList& ad_conversions);
  CreativeAdNotificationList GetUnseenAdsAndRoundRobinIfNeeded(
      const CreativeAdNotificationList& ads) const;
  CreativeAdNotificationList GetUnseenAds(
      const CreativeAdNotificationList& ads) const;
  CreativeAdNotificationList GetAdsForUnseenAdvertisers(
      const CreativeAdNotificationList& ads) const;

  bool IsAdNotificationValid(
      const CreativeAdNotificationInfo& info);
  bool ShowAdNotification(
      const CreativeAdNotificationInfo& info);
  bool IsAllowedToServeAdNotifications();

  uint32_t collect_activity_timer_id_;
  void StartCollectingActivity(
      const uint64_t start_timer_in);
  void CollectActivity();
  void StopCollectingActivity();
  bool IsCollectingActivity() const;

  uint32_t delivering_ad_notifications_timer_id_;
  void StartDeliveringAdNotifications();
  void StartDeliveringAdNotificationsAfterSeconds(
      const uint64_t seconds);
  void DeliverAdNotification();
  void StopDeliveringAdNotifications();
  bool IsDeliveringAdNotifications() const;
  bool IsCatalogOlderThanOneDay();

  #if defined(OS_ANDROID)
  void RemoveAllAdNotificationsAfterReboot();
  void RemoveAllAdNotificationsAfterUpdate();
  #endif

  void BundleUpdated();

  const AdNotificationInfo& get_last_shown_ad_notification() const;
  void set_last_shown_ad_notification(
      const AdNotificationInfo& info);

  void ConfirmAd(
      const AdInfo& info,
      const ConfirmationType confirmation_type);

  void ConfirmAction(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const ConfirmationType confirmation_type);

  void OnTimer(
      const uint32_t timer_id) override;

  uint64_t next_easter_egg_timestamp_in_seconds_;

  void AppendAdNotificationToHistory(
      const AdNotificationInfo& info,
      const ConfirmationType& confirmation_type);

  bool IsCreativeSetFromSampleCatalog(
      const std::string& creative_set_id) const;

  bool IsSupportedUrl(
      const std::string& url) const;

  std::unique_ptr<Client> client_;
  std::unique_ptr<Bundle> bundle_;
  std::unique_ptr<AdsServe> ads_serve_;
  std::unique_ptr<FrequencyCapping> frequency_capping_;
  std::unique_ptr<AdConversions> ad_conversions_;
  std::unique_ptr<usermodel::UserModel> user_model_;
  std::unique_ptr<PurchaseIntentClassifier> purchase_intent_classifier_;

 private:
  bool is_initialized_;

  bool is_confirmations_ready_;

  AdNotificationInfo last_shown_ad_notification_;
  CreativeAdNotificationInfo last_shown_creative_ad_notification_;
  uint32_t sustained_ad_notification_interaction_timer_id_;
  std::string last_sustained_ad_notification_url_;
  void StartSustainingAdNotificationInteraction(
      const uint64_t start_timer_in);
  void SustainAdNotificationInteractionIfNeeded();
  void StopSustainingAdNotificationInteraction();
  bool IsSustainingAdNotificationInteraction() const;
  bool IsStillViewingAdNotification() const;

  std::unique_ptr<AdNotifications> ad_notifications_;

  AdsClient* ads_client_;  // NOT OWNED

  std::vector<std::unique_ptr<PermissionRule>> CreatePermissionRules() const;

  std::vector<std::unique_ptr<ExclusionRule>> CreateExclusionRules() const;

  // Not copyable, not assignable
  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_ADS_IMPL_H_
