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
#include <set>

#include "bat/ads/ads.h"
#include "bat/ads/ads_history.h"
#include "bat/ads/mojom.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/ads_serve.h"
#include "bat/ads/internal/bundle.h"
#include "bat/ads/internal/classification/page_classifier/page_classifier.h"
#include "bat/ads/internal/classification/purchase_intent_classifier/purchase_intent_classifier.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/internal/creative_ad_notification_info.h"
#include "bat/ads/internal/database/database_initialize.h"
#include "bat/ads/internal/database/tables/creative_ad_notifications_database_table.h"
#include "bat/ads/internal/ad_conversions.h"
#include "bat/ads/internal/ad_notification_result_type.h"
#include "bat/ads/internal/ad_notifications.h"
#include "bat/ads/internal/timer.h"

namespace ads {

class Client;
class Bundle;
class AdsServe;
class SubdivisionTargeting;
class AdNotifications;
class AdConversions;
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
  SubdivisionTargeting* get_subdivision_targeting() const;
  AdConversions* get_ad_conversions() const;
  classification::PageClassifier* get_page_classifier() const;

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
  void InitializeStep6(
      const Result result);
  bool IsInitialized();

  void Shutdown(
      ShutdownCallback callback) override;

  bool IsMobile() const;
  bool IsAndroid() const;

  bool is_foreground_;
  void OnForeground() override;
  void OnBackground() override;
  bool IsForeground() const;

  void OnIdle() override;
  void OnUnIdle() override;

  std::set<int32_t> media_playing_;
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

  void LoadPageClassificationUserModel(
      const std::string& locale);
  void LoadPurchaseIntentUserModel(
      const std::string& locale);
  void OnUserModelUpdated(
      const std::string& id) override;
  void LoadPageClassificationUserModelForId(
      const std::string& id);
  void OnLoadPageClassificationUserModelForId(
      const std::string& id,
      const Result result,
      const std::string& json);
  void LoadPurchaseIntentUserModelForId(
      const std::string& id);
  void OnLoadPurchaseIntentUserModelForId(
      const std::string& id,
      const Result result,
      const std::string& json);

  void OnAdsSubdivisionTargetingCodeHasChanged() override;

  void OnPageLoaded(
      const int32_t tab_id,
      const std::string& original_url,
      const std::string& url,
      const std::string& content) override;

  void MaybeSustainAdNotification(
      const int32_t tab_id,
      const std::string& url);

  void ExtractPurchaseIntentSignal(
      const std::string& url);
  void GeneratePurchaseIntentSignalHistoryEntry(
      const classification::PurchaseIntentSignalInfo& purchase_intent_signal);
  classification::PurchaseIntentWinningCategoryList
      GetWinningPurchaseIntentCategories();

  void MaybeClassifyPage(
      const std::string& url,
      const std::string& content);

  void MaybeServeAdNotification(
      const bool should_serve);
  void ServeAdNotificationIfReady(
      const bool should_force);
  void ServeAdNotificationFromCategories(
      const classification::CategoryList& categories);
  void OnServeAdNotificationFromCategories(
      const Result result,
      const classification::CategoryList& categories,
      const CreativeAdNotificationList& ads);
  bool ServeAdNotificationFromParentCategories(
      const classification::CategoryList& categories);
  void ServeUntargetedAdNotification();
  void OnServeUntargetedAdNotification(
      const Result result,
      const classification::CategoryList& categories,
      const CreativeAdNotificationList& ads);
  classification::CategoryList GetCategoriesToServeAd();
  void ServeAdNotificationWithPacing(
      const CreativeAdNotificationList& ads);
  void SuccessfullyServedAd();
  void FailedToServeAdNotification(
      const std::string& reason);

  CreativeAdNotificationList GetEligibleAds(
      const CreativeAdNotificationList& ads);
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

  Timer deliver_ad_notification_timer_;
  void StartDeliveringAdNotifications();
  void StartDeliveringAdNotificationsAfterSeconds(
      const uint64_t seconds);
  void DeliverAdNotification();
  bool IsCatalogOlderThanOneDay();

  #if defined(OS_ANDROID)
  void RemoveAllAdNotificationsAfterReboot();
  void RemoveAllAdNotificationsAfterUpdate();
  #endif

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

  uint64_t next_easter_egg_timestamp_in_seconds_;

  void AppendAdNotificationToHistory(
      const AdNotificationInfo& info,
      const ConfirmationType& confirmation_type);

  std::unique_ptr<Client> client_;
  std::unique_ptr<Bundle> bundle_;
  std::unique_ptr<AdsServe> ads_serve_;
  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<AdConversions> ad_conversions_;
  std::unique_ptr<database::Initialize> database_;
  std::unique_ptr<classification::PageClassifier> page_classifier_;
  std::unique_ptr<classification::PurchaseIntentClassifier>
      purchase_intent_classifier_;

 private:
  bool is_initialized_;

  bool is_confirmations_ready_;

  AdNotificationInfo last_shown_ad_notification_;
  CreativeAdNotificationInfo last_shown_creative_ad_notification_;
  Timer sustain_ad_notification_interaction_timer_;
  std::set<int32_t> sustained_ad_notifications_;
  std::set<int32_t> sustaining_ad_notifications_;
  void MaybeStartSustainingAdNotificationInteraction(
      const int32_t tab_id,
      const std::string& url);
  void SustainAdNotificationInteractionIfNeeded(
      const int32_t tab_id,
      const std::string& url);

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
